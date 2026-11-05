#include "bitmap_support.h"
#include "shape.h"
#include "../engine/resource_file.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <png.h>

namespace megusta { namespace dts {
namespace {
BitmapPaletteDestroy paletteDestroy=nullptr;
BitmapReadPng pngReader=nullptr;
BitmapReadBmp bmpReader=nullptr;
// Engine pixel-format IDs recovered from engineBitmap_resize_impl. Format 2
// is three-byte RGB, 3 is four-byte RGBA, and 6 is 16-bit RGB5551.
constexpr std::uint32_t rgb5551=6;
constexpr std::uint32_t rgb888=2;
constexpr std::uint32_t rgba8888=3;
std::uint32_t pixelBytesPerPixel(std::uint32_t format) {
    if (format==rgb888) return 3;
    if (format==rgba8888) return 4;
    if (format==rgb5551 || format==5) return 2;
    return 1;
}
void dispatchDestroy(EngineBitmapView* bitmap,std::uint8_t releaseObject) {
    FUN_004d2df0(bitmap,releaseObject);
}
EngineBitmapDispatch bitmapDispatch={dispatchDestroy};

void readResourcePng(png_structp png,png_bytep output,png_size_t bytes) {
    auto* file=static_cast<ResourceFile*>(png_get_io_ptr(png));
    if (!file || file->read(output,1,static_cast<std::uint32_t>(bytes))!=bytes)
        png_error(png,"short read from bitmap resource");
}

bool readPngResource(EngineBitmapView* bitmap,void* opaqueFile) {
    auto* file=static_cast<megusta::ResourceFile*>(opaqueFile);
    png_byte signature[8];
    // The saved reader ignores the read count and checks all eight stack bytes.
    // Preserve that ordering for short resources as well as complete files.
    file->read(signature,1,sizeof(signature));
    const bool hasSignature=png_sig_cmp(signature,0,sizeof(signature))==0;
    png_structp png=hasSignature ? png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr) : nullptr;
    png_infop info=png ? png_create_info_struct(png) : nullptr;
    png_infop endInfo=png ? png_create_info_struct(png) : nullptr;
    png_bytep* volatile rows=nullptr;
    volatile bool success=false;
    if (png && info && endInfo && setjmp(png_jmpbuf(png))==0) {
        png_set_read_fn(png,file,readResourcePng);
        png_set_sig_bytes(png,sizeof(signature));
        png_read_info(png,info);
        // 004d71d0 releases any previous pixel planes and palette directly
        // after reading metadata, before querying IHDR or resizing.
        FUN_004d2e90(bitmap);
        png_uint_32 width=0,height=0;
        int bitDepth=0,colorType=0,interlace=0,compression=0,filter=0;
        png_get_IHDR(png,info,&width,&height,&bitDepth,&colorType,&interlace,&compression,&filter);
        int pixelFormat=2;
        if (bitDepth==16) png_set_strip_16(png);
        const bool transparent=png_get_valid(png,info,PNG_INFO_tRNS)!=0;
        if (transparent) png_set_expand(png);
        if (colorType==PNG_COLOR_TYPE_PALETTE) {
            png_set_expand(png);
            pixelFormat=transparent ? 3 : 2;
        } else if (colorType==PNG_COLOR_TYPE_GRAY) {
            png_set_expand(png);
            pixelFormat=4;
        } else if (colorType==PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_expand(png);
            png_set_gray_to_rgb(png);
            pixelFormat=3;
        } else if (colorType==PNG_COLOR_TYPE_RGB) {
            png_set_expand(png);
            pixelFormat=transparent ? 3 : 2;
        } else if (colorType==PNG_COLOR_TYPE_RGB_ALPHA) {
            png_set_expand(png);
            pixelFormat=3;
        }
        png_read_update_info(png,info);
        const png_size_t rowBytes=png_get_rowbytes(png,info);
        resizeEngineBitmap(bitmap,width,height,false,static_cast<std::uint32_t>(pixelFormat));
        // The original allocates this row-pointer array with operator new and
        // has no explicit allocation-failure branch.
        rows=static_cast<png_bytep*>(::operator new(
            static_cast<std::size_t>(height)*sizeof(*rows),std::nothrow));
        for (png_uint_32 y=0;y<height;++y)
            rows[y]=bitmap->pixels+bitmap->mipOffsets[0]+static_cast<std::size_t>(y)*rowBytes;
        png_read_image(png,rows);
        // The recovered reader passes null to png_read_end even though it
        // created a second info structure for png_destroy_read_struct.
        png_read_end(png,nullptr);
        success=true;
    }
    ::operator delete(const_cast<png_bytep*>(rows));
    if (png) png_destroy_read_struct(&png,info ? &info : nullptr,endInfo ? &endInfo : nullptr);
    return success;
}

void reduceBytePixels(const std::uint8_t* source,std::uint8_t* destination,
                      std::uint32_t height,std::uint32_t width,std::uint32_t channels) {
    std::uint32_t outputWidth=width>>1;
    std::uint32_t outputHeight=height>>1;
    if (!outputWidth) outputWidth=1;
    if (!outputHeight) outputHeight=1;
    for (std::uint32_t y=0;y<outputHeight;++y) {
        const std::uint32_t sourceY=y*2;
        const std::uint32_t nextY=(height==1) ? sourceY : sourceY+1;
        for (std::uint32_t x=0;x<outputWidth;++x) {
            const std::uint32_t sourceX=(width==1) ? 0 : x*2;
            const std::uint32_t nextX=(width==1) ? 0 : sourceX+1;
            for (std::uint32_t channel=0;channel<channels;++channel) {
                const std::uint32_t a=source[(sourceY*width+sourceX)*channels+channel];
                const std::uint32_t b=source[(sourceY*width+nextX)*channels+channel];
                const std::uint32_t c=source[(nextY*width+sourceX)*channels+channel];
                const std::uint32_t d=source[(nextY*width+nextX)*channels+channel];
                destination[(y*outputWidth+x)*channels+channel]=
                    static_cast<std::uint8_t>((a+b+c+d)>>2);
            }
        }
    }
}

void clearMipBorder(EngineBitmapView* bitmap,std::uint32_t level) {
    const std::uint32_t width=std::max(bitmap->width>>level,1u);
    const std::uint32_t height=std::max(bitmap->height>>level,1u);
    const std::uint32_t pixelSize=bitmap->bytesPerPixel;
    std::uint8_t* pixels=bitmap->pixels+bitmap->mipOffsets[level];
    const std::uint32_t rowBytes=width*pixelSize;
    if (height<3 || width<3) {
        std::memset(pixels,0,pixelSize*height*width);
        return;
    }
    std::memset(pixels,0,rowBytes-pixelSize);
    std::uint8_t* cursor=pixels-pixelSize;
    while (cursor<pixels+((height-1)*rowBytes-pixelSize)) {
        cursor+=rowBytes;
        std::memset(cursor,0,pixelSize*2);
    }
    std::memset(cursor+pixelSize*2,0,rowBytes-pixelSize);
}

} // namespace

void bindBitmapPaletteDestroy(BitmapPaletteDestroy destroy) { paletteDestroy=destroy; }
void bindBitmapReaders(BitmapReadPng png,BitmapReadBmp bmp) { pngReader=png; bmpReader=bmp; }

void resizeEngineBitmap(EngineBitmapView* bitmap,std::uint32_t width,std::uint32_t height,
                        bool generateMipmaps,std::uint32_t pixelFormat) {
    ::operator delete(bitmap->auxiliaryPixels0);
    bitmap->auxiliaryPixels0=nullptr;
    bitmap->auxiliaryBytes0=0;
    bitmap->auxiliaryMipCount0=0;
    std::fill(bitmap->auxiliaryMipOffsets0,bitmap->auxiliaryMipOffsets0+10,0xffffffffu);
    ::operator delete(bitmap->auxiliaryPixels1);
    bitmap->auxiliaryPixels1=nullptr;
    bitmap->auxiliaryBytes1=0;
    std::fill(bitmap->auxiliaryMipOffsets1,bitmap->auxiliaryMipOffsets1+10,0xffffffffu);

    const std::uint32_t oldBytes=bitmap->pixelBytes;
    std::uint8_t* oldPixels=bitmap->pixels;
    bitmap->pixelFormat=pixelFormat;
    bitmap->width=width;
    bitmap->height=height;
    bitmap->bytesPerPixel=pixelBytesPerPixel(pixelFormat);
    const std::uint32_t pixelSize=bitmap->bytesPerPixel;
    bitmap->mipCount=1;
    bitmap->mipOffsets[0]=0;
    std::uint32_t totalBytes=pixelSize*width*height;
    if (generateMipmaps) {
        do {
            const std::uint32_t levelWidth=width;
            const std::uint32_t levelHeight=height;
            width>>=1;
            height>>=1;
            bitmap->mipOffsets[bitmap->mipCount]=
                bitmap->mipOffsets[bitmap->mipCount-1]+pixelSize*levelWidth*levelHeight;
            if (!width) width=1;
            if (!height) height=1;
            ++bitmap->mipCount;
            totalBytes+=pixelSize*width*height;
        } while (width!=1 || height!=1);
    }
    bitmap->pixelBytes=totalBytes;
    bitmap->pixels=static_cast<std::uint8_t*>(::operator new(totalBytes));
    std::memset(bitmap->pixels,0xff,totalBytes);
    if (oldPixels) {
        std::memcpy(bitmap->pixels,oldPixels,std::min(oldBytes,totalBytes));
        ::operator delete(oldPixels);
    }
}

EngineBitmapView* constructEngineBitmap(EngineBitmapView* bitmap,std::uint32_t width,
        std::uint32_t height,bool generateMipmaps,std::uint32_t pixelFormat) {
    FUN_004d2d90(bitmap);
    resizeEngineBitmap(bitmap,width,height,generateMipmaps,pixelFormat);
    return bitmap;
}

void downsampleRGB5551(const std::uint16_t* source,std::uint16_t* destination,
                       std::uint32_t height,std::uint32_t width) {
    std::uint32_t outputWidth=width>>1,outputHeight=height>>1;
    if (!outputWidth) outputWidth=1;
    if (!outputHeight) outputHeight=1;
    for (std::uint32_t y=0;y<outputHeight;++y) {
        const std::uint32_t sy=y*2,ny=height==1?sy:sy+1;
        for (std::uint32_t x=0;x<outputWidth;++x) {
            const std::uint32_t sx=width==1?0:x*2,nx=width==1?0:sx+1;
            std::uint16_t p[4];
            if (width==1) {
                p[0]=source[sy*width+sx]; p[1]=source[ny*width+sx];
                p[2]=p[3]=0;
            } else {
                p[0]=source[sy*width+sx]; p[1]=source[sy*width+nx];
                p[2]=source[ny*width+sx]; p[3]=source[ny*width+nx];
            }
            const unsigned count=width==1?2:4;
            unsigned red=0,green=0,blue=0;
            for (unsigned n=0;n<count;++n) {
                red+=(p[n]>>11)&0x1f;
                green+=(p[n]>>6)&0x1f;
                blue+=(p[n]>>1)&0x1f;
            }
            destination[y*outputWidth+x]=static_cast<std::uint16_t>(
                ((red/count)<<11)|((green/count)<<6)|((blue/count)<<1)|1u);
        }
    }
}

void downsampleRGB888(const std::uint8_t* source,std::uint8_t* destination,
                      std::uint32_t height,std::uint32_t width) {
    reduceBytePixels(source,destination,height,width,3);
}
void downsampleRGBA8888(const std::uint8_t* source,std::uint8_t* destination,
                        std::uint32_t height,std::uint32_t width) {
    reduceBytePixels(source,destination,height,width,4);
}

void resizeBitmapMipChain(EngineBitmapView* bitmap) {
    const std::uint32_t oldWidth=bitmap->width?bitmap->width:1;
    const std::uint32_t oldHeight=bitmap->height?bitmap->height:1;
    const std::uint32_t pixelSize=bitmap->bytesPerPixel;
    std::uint32_t levels=1,totalBytes=oldWidth*oldHeight*pixelSize;
    bitmap->mipOffsets[0]=0;
    std::uint32_t width=oldWidth,height=oldHeight;
    while (width>1 || height>1) {
        width=std::max(width>>1,1u); height=std::max(height>>1,1u);
        bitmap->mipOffsets[levels]=totalBytes;
        totalBytes+=width*height*pixelSize;
        ++levels;
    }
    std::uint8_t* replacement=static_cast<std::uint8_t*>(::operator new(totalBytes));
    if (bitmap->pixels) {
        const std::uint32_t baseBytes=oldWidth*oldHeight*pixelSize;
        std::memcpy(replacement,bitmap->pixels,std::min(bitmap->pixelBytes,baseBytes));
        ::operator delete(bitmap->pixels);
    }
    bitmap->pixels=replacement;
    bitmap->pixelBytes=totalBytes;
    bitmap->width=oldWidth; bitmap->height=oldHeight; bitmap->mipCount=levels;
}

void generateBitmapMipmaps(EngineBitmapView* bitmap,bool clearBorder) {
    if (bitmap->mipCount==1) resizeBitmapMipChain(bitmap);
    if (bitmap->pixelFormat==rgb5551 || bitmap->pixelFormat==rgb888 ||
        bitmap->pixelFormat==rgba8888) {
        for (std::uint32_t level=1;level<bitmap->mipCount;++level) {
            const std::uint32_t sourceLevel=level-1;
            const std::uint32_t sourceWidth=std::max(bitmap->width>>sourceLevel,1u);
            const std::uint32_t sourceHeight=std::max(bitmap->height>>sourceLevel,1u);
            const std::uint32_t inputOffset=bitmap->mipOffsets[level-1];
            const std::uint32_t outputOffset=bitmap->mipOffsets[level];
            if (bitmap->pixelFormat==rgb5551) {
                // The original 16-bit branch passes the current mip index,
                // unlike its RGB/RGBA branches, which pass the prior level.
                const std::uint32_t currentWidth=std::max(bitmap->width>>level,1u);
                const std::uint32_t currentHeight=std::max(bitmap->height>>level,1u);
                downsampleRGB5551(reinterpret_cast<const std::uint16_t*>(bitmap->pixels+inputOffset),
                    reinterpret_cast<std::uint16_t*>(bitmap->pixels+outputOffset),currentHeight,currentWidth);
            } else if (bitmap->pixelFormat==rgb888)
                downsampleRGB888(bitmap->pixels+inputOffset,bitmap->pixels+outputOffset,sourceHeight,sourceWidth);
            else
                downsampleRGBA8888(bitmap->pixels+inputOffset,bitmap->pixels+outputOffset,sourceHeight,sourceWidth);
        }
    }
    if (clearBorder) for (std::uint32_t level=1;level<bitmap->mipCount;++level)
        clearMipBorder(bitmap,level);
}

void generateNormalMapMipmaps(EngineBitmapView* bitmap) {
    if (bitmap->mipCount==1) resizeBitmapMipChain(bitmap);
    for (std::uint32_t level=1;level<bitmap->mipCount;++level) {
        const std::uint32_t sourceLevel=level-1;
        downsampleRGB888(bitmap->pixels+bitmap->mipOffsets[level-1],
            bitmap->pixels+bitmap->mipOffsets[level],
            std::max(bitmap->height>>sourceLevel,1u),
            std::max(bitmap->width>>sourceLevel,1u));
    }
    for (std::uint32_t level=1;level+1<bitmap->mipCount;++level) {
        std::uint8_t* pixels=bitmap->pixels+bitmap->mipOffsets[level];
        const std::uint32_t count=std::max(bitmap->width>>level,1u)*
                                  std::max(bitmap->height>>level,1u)*3u;
        // Preserve the original unsigned 32-bit multiply and its wraparound:
        // the binary computes ((1 << level) + 0x01ffffff) * 0x80, then shifts.
        const std::uint32_t wrappedBias=((1u<<level)+0x01ffffffu)*0x80u;
        const std::uint32_t bias=wrappedBias>>level;
        for (std::uint32_t i=0;i<count;++i)
            pixels[i]=static_cast<std::uint8_t>((pixels[i]+bias)>>level);
    }
    std::uint8_t* last=bitmap->pixels+bitmap->mipOffsets[bitmap->mipCount-1];
    last[0]=0x80;last[1]=0x80;last[2]=0x80;
}

void convertRGB888ToRGB5551(std::uint8_t* pixels,int pixelCount) {
    auto* output=reinterpret_cast<std::uint16_t*>(pixels);
    for (int i=0;i<pixelCount;++i) {
        const std::uint8_t red=pixels[i*3],green=pixels[i*3+1],blue=pixels[i*3+2];
        output[i]=static_cast<std::uint16_t>(((red>>3)<<11)|((green>>3)<<6)|((blue>>3)<<1)|1u);
    }
}

namespace {
std::uint32_t nextPowerOfTwo(std::uint32_t value) {
    std::uint32_t result=1;
    while (result<value) result<<=1;
    return result;
}
}

EngineBitmapView* expandBitmapPowerOfTwo(EngineBitmapView* bitmap) {
    const std::uint32_t sourceWidth=bitmap->width ? bitmap->width : 1u;
    const std::uint32_t sourceHeight=bitmap->height ? bitmap->height : 1u;
    const std::uint32_t width=nextPowerOfTwo(sourceWidth);
    const std::uint32_t height=nextPowerOfTwo(sourceHeight);
    if (sourceWidth==width && sourceHeight==height) return bitmap;

    auto* expanded=static_cast<EngineBitmapView*>(
        ::operator new(sizeof(EngineBitmapView),std::nothrow));
    if (!expanded) return nullptr;
    constructEngineBitmap(expanded,width,height,false,bitmap->pixelFormat);
    const std::uint32_t rowBytes=sourceWidth*bitmap->bytesPerPixel;
    for (std::uint32_t row=0;row<sourceHeight;++row) {
        std::memcpy(expanded->pixels+expanded->mipOffsets[0]+
                        row*expanded->width*expanded->bytesPerPixel,
                    bitmap->pixels+bitmap->mipOffsets[0]+
                        row*sourceWidth*bitmap->bytesPerPixel,rowBytes);
    }
    return expanded;
}

bool readEngineBitmapPng(EngineBitmapView* bitmap,void* file) {
    BitmapReadPng reader=pngReader ? pngReader : readPngResource;
    return reader(bitmap,file);
}

bool readEngineBitmapBmp(EngineBitmapView* bitmap,void* opaqueFile) {
    if (bmpReader) return bmpReader(bitmap,opaqueFile);
    auto* file=static_cast<megusta::ResourceFile*>(opaqueFile);
    std::uint16_t signature=0, reserved0=0, reserved1=0;
    std::uint32_t fileBytes=0,pixelOffset=0,headerBytes=0;
    file->read(&signature,2,1);
    file->read(&fileBytes,4,1);
    file->read(&reserved0,2,1);
    file->read(&reserved1,2,1);
    file->read(&pixelOffset,4,1);
    file->read(&headerBytes,4,1);
    std::uint32_t width=0,height=0;
    std::uint16_t planes=0,bitsPerPixel=0;
    std::uint32_t compression=0,imageBytes=0;
    std::int32_t horizontalResolution=0,verticalResolution=0;
    std::int32_t colorsUsed=0,importantColors=0;
    file->read(&width,4,1);
    file->read(&height,4,1);
    file->read(&planes,2,1);
    file->read(&bitsPerPixel,2,1);
    file->read(&compression,4,1);
    file->read(&imageBytes,4,1);
    file->read(&horizontalResolution,4,1);
    file->read(&verticalResolution,4,1);
    file->read(&colorsUsed,4,1);
    file->read(&importantColors,4,1);
    std::uint32_t pixelFormat=2;
    if (bitsPerPixel==8) {
        pixelFormat=0;
        if (colorsUsed==0) colorsUsed=256;
        std::uint8_t palette[1024];
        file->read(palette,static_cast<std::uint32_t>(colorsUsed)*4u,1);
    }
    void* scratch=::operator new(width*4u,std::nothrow);
    resizeEngineBitmap(bitmap,width,height,false,pixelFormat);
    const std::uint32_t rowWidth=bitmap->width ? bitmap->width : 1;
    const std::uint32_t rowCount=bitmap->height ? bitmap->height : 1;
    for (std::int32_t row=static_cast<std::int32_t>(height)-1;row>=0;--row) {
        const std::uint32_t rowBytes=bitmap->bytesPerPixel*rowWidth;
        file->read(bitmap->pixels+bitmap->mipOffsets[0]+
                   static_cast<std::uint32_t>(row)*rowBytes,rowBytes,1);
    }
    if (bitmap->bytesPerPixel==3) {
        std::uint8_t* pixel=bitmap->pixels+bitmap->mipOffsets[0];
        for (std::uint32_t i=0;i<rowCount*rowWidth;++i,pixel+=3)
            std::swap(pixel[0],pixel[2]);
    }
    ::operator delete(scratch);
    return true;
}

} }

extern "C" megusta::dts::EngineBitmapView* FUN_004d2d90(megusta::dts::EngineBitmapView* bitmap) {
    using namespace megusta::dts;
    bitmap->vtable=&bitmapDispatch;
    bitmap->pixelFormat=2;
    bitmap->pixels=nullptr;bitmap->pixelBytes=0;bitmap->width=0;bitmap->height=0;
    bitmap->bytesPerPixel=0;bitmap->mipCount=0;bitmap->palette=nullptr;
    bitmap->auxiliaryPixels0=nullptr;bitmap->auxiliaryBytes0=0;bitmap->auxiliaryMipCount0=0;
    bitmap->auxiliaryPixels1=nullptr;bitmap->auxiliaryBytes1=0;
    for (int i=0;i<10;++i) {
        bitmap->mipOffsets[i]=0xffffffffu;
        bitmap->auxiliaryMipOffsets0[i]=0xffffffffu;
        bitmap->auxiliaryMipOffsets1[i]=0xffffffffu;
    }
    return bitmap;
}


extern "C" void FUN_004d2e90(megusta::dts::EngineBitmapView* bitmap) {
    using namespace megusta::dts;
    ::operator delete(bitmap->pixels);
    bitmap->pixels=nullptr;bitmap->pixelBytes=0;bitmap->width=0;bitmap->height=0;
    bitmap->mipCount=0;
    if (bitmap->palette) {
        if (paletteDestroy) paletteDestroy(bitmap->palette);
        else ::operator delete(bitmap->palette);
    }
    bitmap->palette=nullptr;
    ::operator delete(bitmap->auxiliaryPixels0);
    bitmap->auxiliaryPixels0=nullptr;bitmap->auxiliaryBytes0=0;bitmap->auxiliaryMipCount0=0;
    ::operator delete(bitmap->auxiliaryPixels1);
    bitmap->auxiliaryPixels1=nullptr;bitmap->auxiliaryBytes1=0;
}

extern "C" void FUN_004d2e80(megusta::dts::EngineBitmapView* bitmap) {
    bitmap->vtable=&megusta::dts::bitmapDispatch;
    FUN_004d2e90(bitmap);
}

extern "C" void* FUN_004d2df0(megusta::dts::EngineBitmapView* bitmap,std::uint8_t deleteObject) {
    FUN_004d2e80(bitmap);
    if (deleteObject&1u) ::operator delete(bitmap);
    return bitmap;
}

extern "C" void FUN_004d3080(std::uint16_t* source,std::uint16_t* destination,
                              std::uint32_t height,std::uint32_t width) {
    megusta::dts::downsampleRGB5551(source,destination,height,width);
}
extern "C" void FUN_004d3270(std::uint8_t* source,std::uint8_t* destination,
                              std::uint32_t height,std::uint32_t width) {
    megusta::dts::downsampleRGB888(source,destination,height,width);
}
extern "C" void FUN_004d3390(std::uint8_t* source,std::uint8_t* destination,
                              std::uint32_t height,std::uint32_t width) {
    megusta::dts::downsampleRGBA8888(source,destination,height,width);
}
extern "C" void MEGUSTA_DTS_THISCALL FUN_004d34f0(
        megusta::dts::EngineBitmapView* bitmap,char clearBorder) {
    megusta::dts::generateBitmapMipmaps(bitmap,clearBorder!=0);
}
extern "C" void MEGUSTA_DTS_FASTCALL FUN_004d3730(
        megusta::dts::EngineBitmapView* bitmap) {
    megusta::dts::generateNormalMapMipmaps(bitmap);
}

namespace {
megusta::dts::EngineBitmapView* createBitmap() {
    void* memory=::operator new(sizeof(megusta::dts::EngineBitmapView),std::nothrow);
    if (!memory) return nullptr;
    auto* bitmap=static_cast<megusta::dts::EngineBitmapView*>(memory);
    return FUN_004d2d90(bitmap);
}
}
extern "C" megusta::dts::EngineBitmapView* FUN_004d38d0(void* file) {
    using namespace megusta::dts;
    EngineBitmapView* bitmap=createBitmap();
    if (readEngineBitmapPng(bitmap,file)) return bitmap;
    if (bitmap) bitmap->vtable->destroy(bitmap,1);
    return nullptr;
}
extern "C" megusta::dts::EngineBitmapView* FUN_004d3920(void* file) {
    using namespace megusta::dts;
    EngineBitmapView* bitmap=createBitmap();
    if (readEngineBitmapBmp(bitmap,file)) return bitmap;
    if (bitmap) bitmap->vtable->destroy(bitmap,1);
    return nullptr;
}
