#include "texture_manager.h"
#include "bitmap_support.h"
#include "mesh.h"
#include "shape.h"
#include "../engine/resource_file.h"
#include "../engine/opengl_adapter.h"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

namespace megusta { namespace dts {

EngineTextureEntry* textureEntryList=nullptr;
EngineTextureEntry** textureEntryBuckets=nullptr;
std::uint32_t textureEntryBucketCount=0;

namespace {
void defaultDestroy(EngineTextureEntry* entry) {
    destroyTextureEntry(entry);
}
TextureEntryDestroy destroyEntry=defaultDestroy;
TextureFatalExit fatalExit=nullptr;
TextureBitmapLoader bitmapLoader=nullptr;
std::uint32_t maximumDownloadedMip=0;

std::uint32_t powerOfTwo(std::uint32_t value) {
    std::uint32_t result=1;
    while (result<value) result<<=1;
    return result;
}

EngineTextureEntry* allocateTextureEntry(const char* filename,int kind) {
    const std::size_t filenameBytes=filename ? std::strlen(filename)+1 : 0;
    void* memory=::operator new(sizeof(EngineTextureEntry)+filenameBytes,std::nothrow);
    if (!memory) return nullptr;
    auto* entry=new (memory) EngineTextureEntry;
    if (filename) {
        auto* storedName=reinterpret_cast<char*>(entry+1);
        std::memcpy(storedName,filename,filenameBytes);
        entry->filename=storedName;
    }
    entry->kind=kind;
    linkTextureEntry(entry);
    return entry;
}

void destroyBitmap(EngineBitmapView* bitmap) {
    if (bitmap && bitmap->vtable && bitmap->vtable->destroy)
        bitmap->vtable->destroy(bitmap,1);
}

bool mapPixelFormat(const EngineBitmapView* bitmap,int* format,int* internalFormat,
                    int* pixelType) {
    *pixelType=0;
    switch (bitmap->pixelFormat) {
    case 0: case 1: *format=0x40; break;
    case 2: *format=0x3e; break;
    case 3: *format=0x3f; break;
    case 4: *format=0x42; break;
    // The saved mapper sets this error token and calls fatal exit for formats 5/6.
    case 5: case 6:
        *format=0x3f;
        *pixelType=0x8034;
        if (fatalExit) fatalExit();
        else std::abort();
        return false;
    default: *format=0; *internalFormat=0; return false;
    }
    *internalFormat=*format;
    return true;
}
}

void bindTextureEntryDestroy(TextureEntryDestroy destroy) {
    destroyEntry=destroy ? destroy : defaultDestroy;
}
void bindTextureFatalExit(TextureFatalExit callback) { fatalExit=callback; }
void bindTextureBitmapLoader(TextureBitmapLoader loader) { bitmapLoader=loader; }

bool mapTexturePixelFormat(const EngineBitmapView* bitmap,int* format,int* internalFormat,
                           int* pixelType) {
    return mapPixelFormat(bitmap,format,internalFormat,pixelType);
}

EngineTextureEntry* findTextureEntry(const char* filename,int kind) {
    const auto hash=textureFilenameHash(reinterpret_cast<const std::uint8_t*>(filename));
    EngineTextureEntry* entry=textureEntryBuckets[hash%textureEntryBucketCount];
    while (entry && (entry->kind!=kind || !entry->filename ||
                     std::strcmp(entry->filename,filename)!=0))
        entry=entry->hashNext;
    return entry;
}

void retainTextureEntry(EngineTextureEntry* entry) {
    if (entry) ++entry->referenceCount;
}

EngineTextureEntry* releaseTextureEntry(EngineTextureEntry* entry) {
    if (!entry) return nullptr;
    --entry->referenceCount;
    if (entry->referenceCount==0) {
        destroyTextureEntry(entry);
        return nullptr;
    }
    return entry;
}

void updateTexturePixels(EngineTextureEntry* entry) {
    EngineBitmapView* bitmap=entry->bitmap;
    int format=0,internalFormat=0,pixelType=0;
    if (!mapPixelFormat(bitmap,&format,&internalFormat,&pixelType) || !dtsRenderGL) return;
    dtsRenderGL->bindTexture(0x4a,entry->textureName);
    EngineBitmapView* expanded=expandBitmapPowerOfTwo(bitmap);
    if (!expanded) return;
    for (std::uint32_t mip=0;mip<expanded->mipCount;++mip) {
        const auto width=std::max(expanded->width>>mip,1u);
        const auto height=std::max(expanded->height>>mip,1u);
        dtsRenderGL->texSubImage2D(0x4a,static_cast<int>(mip),0,0,
            static_cast<int>(width),static_cast<int>(height),format,pixelType,
            expanded->pixels+expanded->mipOffsets[mip]);
    }
    if (expanded!=bitmap) expanded->vtable->destroy(expanded,1);
}

std::uint32_t textureFilenameHash(const std::uint8_t* filename) {
    std::uint32_t hash=0;
    for (std::uint8_t value=*filename; value; value=*++filename)
        hash+=value&0x1fu;
    return hash;
}

void initializeTextureEntryList() {
    textureEntryList=nullptr;
    textureEntryBucketCount=0x100;
    textureEntryBuckets=static_cast<EngineTextureEntry**>(::operator new(
        sizeof(EngineTextureEntry*)*textureEntryBucketCount));
    for (std::uint32_t bucket=0;bucket<textureEntryBucketCount;++bucket)
        textureEntryBuckets[bucket]=nullptr;
}

void unlinkTextureEntry(EngineTextureEntry* entry) {
    if (entry->next) entry->next->previous=entry->previous;
    if (!entry->previous) textureEntryList=entry->next;
    else entry->previous->next=entry->next;
    if (entry->filename) {
        const std::uint32_t bucket=textureFilenameHash(
            reinterpret_cast<const std::uint8_t*>(entry->filename))%textureEntryBucketCount;
        EngineTextureEntry** link=textureEntryBuckets+bucket;
        EngineTextureEntry* current=*link;
        while (current && current!=entry) {
            link=&current->hashNext;
            current=current->hashNext;
        }
        if (current) *link=entry->hashNext;
    }
}

void linkTextureEntry(EngineTextureEntry* entry) {
    entry->next=textureEntryList;
    entry->previous=nullptr;
    if (textureEntryList) textureEntryList->previous=entry;
    textureEntryList=entry;
    if (entry->filename) {
        const std::uint32_t bucket=textureFilenameHash(
            reinterpret_cast<const std::uint8_t*>(entry->filename))%textureEntryBucketCount;
        entry->hashNext=textureEntryBuckets[bucket];
        textureEntryBuckets[bucket]=entry;
    }
}

void destroyTextureEntryList() {
    while (textureEntryList) destroyEntry(textureEntryList);
    ::operator delete(textureEntryBuckets);
    textureEntryBuckets=nullptr;
    // The saved cleanup clears the bucket pointer but retains the 0x100 count.
}

std::uint32_t uploadBitmapMipChain(EngineBitmapView* bitmap,bool clamp,
                                   std::uint32_t firstMip) {
    if (!bitmap || !dtsRenderGL) return 0;
    std::uint32_t texture=0;
    dtsRenderGL->generateTextures(1,&texture);
    dtsRenderGL->bindTexture(0x4a,texture);
    int format=0,internalFormat=0,pixelType=0;
    if (!mapPixelFormat(bitmap,&format,&internalFormat,&pixelType)) return 0;
    EngineBitmapView* expanded=expandBitmapPowerOfTwo(bitmap);
    if (!expanded) {
        dtsRenderGL->deleteTextures(1,&texture);
        return 0;
    }
    for (std::uint32_t mip=firstMip;mip<expanded->mipCount;++mip) {
        const std::uint32_t width=std::max(expanded->width>>mip,1u);
        const std::uint32_t height=std::max(expanded->height>>mip,1u);
        dtsRenderGL->textureImage2D(0x4a,static_cast<int>(mip-firstMip),internalFormat,
            static_cast<int>(width),static_cast<int>(height),0,format,pixelType,
            expanded->pixels+expanded->mipOffsets[mip],0);
    }
    if (expanded!=bitmap) expanded->vtable->destroy(expanded,1);

    dtsRenderGL->texParameteri(0x4a,0x4d,0x50);
    dtsRenderGL->texParameteri(0x4a,0x4e,bitmap->mipCount==1 ? 0x50 : 0x53);
    const int wrap=clamp ? 0x59 : 0x57;
    dtsRenderGL->texParameteri(0x4a,0x4b,wrap);
    dtsRenderGL->texParameteri(0x4a,0x4c,wrap);
    return texture;
}

EngineTextureEntry* createDynamicTextureEntry(const char* filename,
        EngineBitmapView* bitmap,bool clamp) {
    if (!bitmap) return nullptr;
    auto* entry=allocateTextureEntry(filename,2);
    if (!entry) return nullptr;
    entry->bitmap=bitmap;
    entry->bitmapWidth=bitmap->width ? bitmap->width : 1u;
    entry->bitmapHeight=bitmap->height ? bitmap->height : 1u;
    entry->textureWidth=powerOfTwo(entry->bitmapWidth);
    entry->textureHeight=powerOfTwo(entry->bitmapHeight);
    entry->downloadWidth=entry->textureWidth;
    entry->downloadHeight=entry->textureHeight;
    entry->clamp=clamp ? 1u : 0u;
    entry->textureName=uploadBitmapMipChain(bitmap,clamp,0);
    return entry;
}

EngineTextureEntry* createOrReplaceTextureEntry(const char* filename,
        EngineBitmapView* bitmap,int kind,bool clamp) {
    if (!bitmap) return nullptr;
    EngineTextureEntry* entry=filename ? findTextureEntry(filename,kind) : nullptr;
    const bool isNewEntry=entry==nullptr;
    if (entry) {
        if (entry->bitmap!=bitmap) destroyBitmap(entry->bitmap);
        if (dtsRenderGL) {
            if (entry->textureName) dtsRenderGL->deleteTextures(1,&entry->textureName);
            if (entry->alternateTextureName) dtsRenderGL->deleteTextures(1,&entry->alternateTextureName);
        }
        entry->textureName=0;
        entry->alternateTextureName=0;
    } else {
        entry=allocateTextureEntry(filename,kind);
        if (!entry) return nullptr;
    }
    if (isNewEntry) {
        entry->referenceCount=0;
        entry->kind=kind;
    }
    entry->bitmap=bitmap;
    entry->bitmapWidth=bitmap->width ? bitmap->width : 1u;
    entry->bitmapHeight=bitmap->height ? bitmap->height : 1u;
    entry->textureWidth=powerOfTwo(entry->bitmapWidth);
    entry->textureHeight=powerOfTwo(entry->bitmapHeight);
    entry->clamp=clamp ? 1u : 0u;

    if (kind==5) generateNormalMapMipmaps(bitmap);
    else if (kind!=0 && kind!=1) generateBitmapMipmaps(bitmap,kind==6);

    if (!entry->textureName) {
        std::uint32_t firstMip=0;
        if (bitmap->mipCount>1 && kind!=5) {
            firstMip=std::min(maximumDownloadedMip,bitmap->mipCount-1);
        }
        entry->downloadWidth=std::max(entry->bitmapWidth>>firstMip,1u);
        entry->downloadHeight=std::max(entry->bitmapHeight>>firstMip,1u);
        entry->textureName=uploadBitmapMipChain(bitmap,clamp,firstMip);
    }

    if (kind!=4) {
        if (kind==1 || kind==7) return entry;
        destroyBitmap(entry->bitmap);
        entry->bitmap=nullptr;
        return entry;
    }
    if (bitmap->width>0x40u) {
        auto* small=static_cast<EngineBitmapView*>(
            ::operator new(sizeof(EngineBitmapView),std::nothrow));
        if (small) {
            constructEngineBitmap(small,0x40,0x40,true,2);
            const std::uint32_t first=bitmap->mipCount-small->mipCount;
            std::memcpy(small->pixels+small->mipOffsets[0],
                        bitmap->pixels+bitmap->mipOffsets[first],small->pixelBytes);
            destroyBitmap(entry->bitmap);
            entry->bitmap=small;
            entry->alternateTextureName=uploadBitmapMipChain(small,clamp,0);
        }
    } else {
        entry->alternateTextureName=entry->textureName;
    }
    return entry;
}

EngineTextureEntry* loadTextureByName(const char* filename,int kind,bool clamp) {
    if (!filename) return nullptr;
    if (EngineTextureEntry* cached=findTextureEntry(filename,kind)) return cached;

    std::string path=iflBaseTextures ? iflBaseTextures : "";
    path+=filename;
    EngineBitmapView* bitmap=bitmapLoader ? bitmapLoader(path.c_str()) : nullptr;
    if (!bitmap) {
        static const char* extensions[]={".png",".tga",".bmp",nullptr};
        for (const char** extension=extensions;*extension && !bitmap;++extension) {
            std::string candidate=path+*extension;
            if (std::strcmp(*extension,".png")==0) {
                if (iflOpenFile) {
                    ResourceFile* file=iflOpenFile(candidate.c_str(),"rb");
                    if (file) {
                        bitmap=FUN_004d38d0(file);
                        if (iflCloseFile) iflCloseFile(file);
                    }
                }
            } else if (std::strcmp(*extension,".bmp")==0 && iflOpenFile) {
                ResourceFile* file=iflOpenFile(candidate.c_str(),"rb");
                if (file) {
                    bitmap=FUN_004d3920(file);
                    if (iflCloseFile) iflCloseFile(file);
                }
            } else if (bitmapLoader) {
                bitmap=bitmapLoader(candidate.c_str());
            }
        }
    }
    return bitmap ? createOrReplaceTextureEntry(filename,bitmap,kind,clamp) : nullptr;
}

void destroyTextureEntry(EngineTextureEntry* entry) {
    if (!entry) return;
    if (dtsRenderGL) {
        if (entry->alternateTextureName && entry->alternateTextureName!=entry->textureName)
            dtsRenderGL->deleteTextures(1,&entry->alternateTextureName);
        if (entry->textureName) dtsRenderGL->deleteTextures(1,&entry->textureName);
    }
    destroyBitmap(entry->bitmap);
    unlinkTextureEntry(entry);
    entry->~EngineTextureEntry();
    ::operator delete(entry);
}

} }

extern "C" void FUN_004cd9f0(void) {
    megusta::dts::initializeTextureEntryList();
}
extern "C" void FUN_004cda90(megusta::dts::EngineTextureEntry* entry) {
    megusta::dts::unlinkTextureEntry(entry);
}
extern "C" void FUN_004cdb00(megusta::dts::EngineTextureEntry* entry) {
    megusta::dts::linkTextureEntry(entry);
}
extern "C" void FUN_004cdb60(void) {
    megusta::dts::destroyTextureEntryList();
}
extern "C" int FUN_004ce4e0(const std::uint8_t* filename) {
    return static_cast<int>(megusta::dts::textureFilenameHash(filename));
}
