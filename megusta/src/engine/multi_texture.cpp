#include "multi_texture.h"
#include "pix_texture.h"
#include "resource_mount.h"
#include "resource_runtime.h"
#include "reader_stream.h"
#include "ul.h"
#include <GL/gl.h>
#include <GL/glpng.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstring>
#include "zlib.h"
namespace megusta::pix {
namespace {
MultiTextureList* allocate(std::uint32_t count) {
    auto* result=new MultiTextureList;
    result->textureCount=count;
    result->textureNames.reset(new std::uint32_t[count]{});
    return result;
}
std::string numbered(const std::string& stem,unsigned index,const char* extension) {
    char suffix[40]; std::snprintf(suffix,sizeof(suffix),"%02d.%s",static_cast<int>(index),extension);
    return stem+suffix;
}
}
static std::uint32_t bindTextureFile(ResourceMountTable* mounts,const char* path,int mipmaps,int wrap,int minFilter,int magFilter,int* outWidth=nullptr,int* outHeight=nullptr,bool useBound=false,int* alpha=nullptr) {
    if(outWidth)*outWidth=0;if(outHeight)*outHeight=0;
    auto open=[&](const char* name) { return mounts?mounts->open(name,"rb"):
        std::unique_ptr<ResourceFile>(sharedIo?sharedIo->open(name,"rb"):nullptr); };
    auto file=open(path);
    pngInfo info{};
    if (!file) {
        // Shipped atlases commonly contain TX8 rather than their PNG source.
        // Resolve that physical representation through the mounted PIX reader.
        std::string packed(path); const auto dot=packed.rfind('.');
        if(dot!=std::string::npos)packed.replace(dot,std::string::npos,".TX8");
        file=open(packed.c_str());
        if(!file) { std::fprintf(stderr,"Mounted texture not found: %s\n",path);return 0; }
        std::uint32_t texture=0;
        if(useBound){GLint bound;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);texture=bound;}
        else {glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);}
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,minFilter);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,magFilter);
        // Shipped TX8 v123456790: ten little-endian words, followed by a
        // zlib payload containing a 256-entry RGBA palette and index levels.
        // This is a platform compatibility reader, not the distinct PIX ABI.
        std::uint32_t header[10];
        if(file->read(header,sizeof(header),1)==1 && header[0]==123456790u &&
           (header[3]==3 || header[3]==4) && header[8]==header[4]+1024u) {
            std::vector<unsigned char> packed(header[9]),decoded(header[8]);
            uLongf length=decoded.size();
            if(file->read(packed.data(),1,packed.size())==packed.size() &&
               uncompress(decoded.data(),&length,packed.data(),packed.size())==Z_OK && length==decoded.size()) {
                std::uint32_t width=header[1],height=header[2];std::size_t offset=1024;
                for(std::uint32_t level=0;level<header[5];++level) {
                    const std::size_t count=std::size_t(width)*height;
                    if(offset+count>decoded.size())break;
                    std::vector<unsigned char> rgba(count*4);
                    for(std::size_t i=0;i<count;++i) {
                        const unsigned char* color=&decoded[decoded[offset+i]*4];
                        std::memcpy(&rgba[i*4],color,4);
                        if(header[3]==3)rgba[i*4+3]=255;
                    }
                    glTexImage2D(GL_TEXTURE_2D,level,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
                    offset+=count; width=width>1?width/2:1;height=height>1?height/2:1;
                    if(mipmaps==PNG_NOMIPMAPS)break;
                }
                if(outWidth)*outWidth=header[1];if(outHeight)*outHeight=header[2];
                if(alpha)*alpha=header[3]==4?8:0;
                return texture;
            }
        }
        if(!useBound)glDeleteTextures(1,&texture);return 0;
    }
    auto reader=resourceReader(*file);
    std::uint32_t texture=0;
    if(useBound){GLint bound;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);if(pngLoadReader(&reader,mipmaps,PNG_ALPHA,&info))texture=bound;}
    else texture=pngBindReader(&reader,mipmaps,PNG_ALPHA,&info,wrap,minFilter,magFilter);
    if(alpha)*alpha=texture?info.Alpha:0;
    if(texture){if(outWidth)*outWidth=info.Width;if(outHeight)*outHeight=info.Height;}
    if(!texture)std::fprintf(stderr,"Mounted PNG decode failed: %s\n",path);
    return texture;
}
std::uint32_t bindPngTexture(ResourceMountTable& mounts,const char* path,int mipmaps,int wrap,int minFilter,int magFilter,int* width,int* height) {
    return bindTextureFile(&mounts,path,mipmaps,wrap,minFilter,magFilter,width,height);
}
std::uint32_t bindPngTexture(const char* path,int mipmaps,int wrap,int minFilter,int magFilter) {
    return bindTextureFile(nullptr,path,mipmaps,wrap,minFilter,magFilter);
}
bool loadBoundTexture(const char* path,int* width,int* height,int* alpha) {
    return bindTextureFile(nullptr,path,PNG_BUILDMIPMAPS,GL_REPEAT,GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR,width,height,true,alpha)!=0;
}
// 00462e60. Engine GL tokens are translated by the caller to actual GL enums.
MultiTextureList* loadMultiTexture(ResourceMountTable& mounts,const char* path,bool external,
    int mipmaps,int wrap,int minFilter,int magFilter) {
    if (!external) {
        auto file=mounts.open(path,"rb");
        if (file) {
            MultiTextureList* textures=nullptr;
            std::uint32_t count=1;
            for (std::uint32_t index=0;index<count;++index) {
                struct Header { std::uint32_t magic,count,unknown[5]; } header{};
                static_assert(sizeof(Header)==0x1c,"MPX record header");
                if (file->read(&header,sizeof(header),1)!=1 || header.magic!=0xfeedfaceu) std::abort();
                if (index==0) {
                    count=header.count;
                    if (static_cast<std::int32_t>(count)<1) std::abort();
                    textures=allocate(count);
                }
                glGenTextures(1,&textures->textureNames[index]);
                glBindTexture(GL_TEXTURE_2D,textures->textureNames[index]);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,magFilter);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,minFilter);
                auto reader=resourceReader(*file);
                load(&reader,nullptr,mipmaps!=0);
            }
            return textures;
        }
    }
    std::string name(path);
    auto dot=name.rfind('.');
    if (dot==std::string::npos) return nullptr;
    std::string stem=name.substr(0,dot);
    ResourceStatus status{};
    std::uint32_t count=0;
    while (mounts.queryStat(numbered(stem,count,"TX8").c_str(),status)==0) ++count;
    if (count==0) while (mounts.queryStat(numbered(stem,count,"PNG").c_str(),status)==0) ++count;
    auto* textures=allocate(count);
    for (std::uint32_t index=0;index<count;++index) {
        // The saved body uses PNG even when TX8 discovery supplied the count.
        // Preserve that behavior; do not silently invent a TX8 replacement.
        textures->textureNames[index]=bindPngTexture(mounts,numbered(stem,index,"PNG").c_str(),
            mipmaps?PNG_BUILDMIPMAPS:PNG_NOMIPMAPS,wrap,minFilter,magFilter);
    }
    return textures;
}
std::uint32_t getMultiTexture(const MultiTextureList* textures,int index) { // 00463130
    if (textures && index>=0 && index<static_cast<std::int32_t>(textures->textureCount))
        return textures->textureNames[index];
    ulSetError(UL_WARNING,"Invalid GetMultiGL() parms");
    return 0;
}
void destroyMultiTexture(MultiTextureList* textures) {
    if (!textures) return;
    if (textures->textureNames && textures->textureCount)
        glDeleteTextures(static_cast<GLsizei>(textures->textureCount),textures->textureNames.get());
    delete textures;
}
}
