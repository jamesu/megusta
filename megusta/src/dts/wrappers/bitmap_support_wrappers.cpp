#include "bitmap_support_wrappers.h"
using namespace megusta::dts;

extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_THISCALL engineBitmap_ctor_impl(
        megusta::dts::EngineBitmapView* bitmap,std::uint32_t width,std::uint32_t height,
        char generateMipmaps,std::uint32_t pixelFormat) {
    return megusta::dts::constructEngineBitmap(bitmap,width,height,generateMipmaps!=0,pixelFormat);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL engineBitmap_readPng_impl(
        megusta::dts::EngineBitmapView* bitmap,void* file) {
    return megusta::dts::readEngineBitmapPng(bitmap,file) ? 1u : 0u;
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL engineBitmap_readBmp_impl(
        megusta::dts::EngineBitmapView* bitmap,void* opaqueFile) {
    return megusta::dts::readEngineBitmapBmp(bitmap,opaqueFile) ? 1u : 0u;
}

extern "C" void MEGUSTA_DTS_THISCALL engineBitmap_resize_impl(
        megusta::dts::EngineBitmapView* bitmap,std::uint32_t width,std::uint32_t height,
        char generateMipmaps,std::uint32_t pixelFormat) {
    megusta::dts::resizeEngineBitmap(bitmap,width,height,generateMipmaps!=0,pixelFormat);
}

extern "C" void DTS_support_RGB888_to_RGB5551_impl(std::uint16_t* pixels,int pixelCount) {
    megusta::dts::convertRGB888ToRGB5551(reinterpret_cast<std::uint8_t*>(pixels),pixelCount);
}

extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_CDECL
engineBitmap_expandPowerOfTwo_impl(megusta::dts::EngineBitmapView* bitmap) {
    return megusta::dts::expandBitmapPowerOfTwo(bitmap);
}
