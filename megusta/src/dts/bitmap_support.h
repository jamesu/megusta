#pragma once
#include <cstddef>
#include <cstdint>
#include "vector_storage.h"

namespace megusta { namespace dts {

struct EngineBitmapView;
struct EngineBitmapDispatch {
    void (*destroy)(EngineBitmapView*,std::uint8_t);
};

// Recovered 0xb0-byte bitmap record. The constructor's three ten-word loops
// place mip offsets at words 8, 21 and 34; resize sets bytesPerPixel at word 6
// and mipCount at word 7. Pixel-format IDs are engine values.
struct EngineBitmapView {
    EngineBitmapDispatch* vtable;
    std::uint32_t pixelFormat;
    std::uint8_t* pixels;
    std::uint32_t pixelBytes,width,height,bytesPerPixel,mipCount;
    std::uint32_t mipOffsets[10];
    void* palette;
    std::uint8_t* auxiliaryPixels0;
    std::uint32_t auxiliaryBytes0;
    std::uint32_t auxiliaryMipOffsets0[10];
    std::uint32_t auxiliaryMipCount0;
    std::uint8_t* auxiliaryPixels1;
    std::uint32_t auxiliaryBytes1;
    std::uint32_t auxiliaryMipOffsets1[10];
};
static_assert(sizeof(EngineBitmapView)==0xb0,"recovered 32-bit bitmap object size");
static_assert(offsetof(EngineBitmapView,bytesPerPixel)==0x18,"recovered bitmap stride offset");
static_assert(offsetof(EngineBitmapView,mipCount)==0x1c,"recovered bitmap mip-count offset");
static_assert(offsetof(EngineBitmapView,mipOffsets)==0x20,"recovered bitmap mip offsets");
static_assert(offsetof(EngineBitmapView,palette)==0x48,"recovered bitmap palette offset");
static_assert(offsetof(EngineBitmapView,auxiliaryPixels0)==0x4c,"recovered first auxiliary plane");
static_assert(offsetof(EngineBitmapView,auxiliaryMipOffsets0)==0x54,"recovered first auxiliary offsets");
static_assert(offsetof(EngineBitmapView,auxiliaryMipCount0)==0x7c,"recovered auxiliary mip-count offset");
static_assert(offsetof(EngineBitmapView,auxiliaryPixels1)==0x80,"recovered second auxiliary plane");
static_assert(offsetof(EngineBitmapView,auxiliaryMipOffsets1)==0x88,"recovered second auxiliary offsets");

using BitmapPaletteDestroy = void (*)(void* palette);
using BitmapReadPng = bool (*)(EngineBitmapView*,void* file);
using BitmapReadBmp = bool (*)(EngineBitmapView*,void* file);
void bindBitmapPaletteDestroy(BitmapPaletteDestroy destroy);
void bindBitmapReaders(BitmapReadPng pngReader,BitmapReadBmp bmpReader);
void downsampleRGB5551(const std::uint16_t* source,std::uint16_t* destination,
                       std::uint32_t height,std::uint32_t width);
void downsampleRGB888(const std::uint8_t* source,std::uint8_t* destination,
                      std::uint32_t height,std::uint32_t width);
void downsampleRGBA8888(const std::uint8_t* source,std::uint8_t* destination,
                        std::uint32_t height,std::uint32_t width);
void resizeBitmapMipChain(EngineBitmapView* bitmap);
void resizeEngineBitmap(EngineBitmapView* bitmap,std::uint32_t width,std::uint32_t height,
                        bool generateMipmaps,std::uint32_t pixelFormat);
EngineBitmapView* constructEngineBitmap(EngineBitmapView* bitmap,std::uint32_t width,
                        std::uint32_t height,bool generateMipmaps,std::uint32_t pixelFormat);
bool readEngineBitmapPng(EngineBitmapView* bitmap,void* file);
bool readEngineBitmapBmp(EngineBitmapView* bitmap,void* file);
void generateBitmapMipmaps(EngineBitmapView* bitmap,bool clearBorder);
void generateNormalMapMipmaps(EngineBitmapView* bitmap);
void convertRGB888ToRGB5551(std::uint8_t* pixels,int pixelCount);
// 004cdc20: returns the input view when its dimensions are already powers of
// two, otherwise creates a padded view and copies each source row unchanged.
EngineBitmapView* expandBitmapPowerOfTwo(EngineBitmapView* bitmap);

} }

extern "C" {
megusta::dts::EngineBitmapView* FUN_004d2d90(megusta::dts::EngineBitmapView* bitmap);
void* FUN_004d2df0(megusta::dts::EngineBitmapView* bitmap,std::uint8_t deleteObject);
void FUN_004d2e80(megusta::dts::EngineBitmapView* bitmap);
void FUN_004d2e90(megusta::dts::EngineBitmapView* bitmap);
void FUN_004d3080(std::uint16_t* source,std::uint16_t* destination,std::uint32_t height,std::uint32_t width);
void FUN_004d3270(std::uint8_t* source,std::uint8_t* destination,std::uint32_t height,std::uint32_t width);
void FUN_004d3390(std::uint8_t* source,std::uint8_t* destination,std::uint32_t height,std::uint32_t width);
void MEGUSTA_DTS_THISCALL FUN_004d34f0(
    megusta::dts::EngineBitmapView* bitmap,char clearBorder);
void MEGUSTA_DTS_FASTCALL FUN_004d3730(
    megusta::dts::EngineBitmapView* bitmap);
megusta::dts::EngineBitmapView* FUN_004d38d0(void* file);
megusta::dts::EngineBitmapView* FUN_004d3920(void* file);
}
