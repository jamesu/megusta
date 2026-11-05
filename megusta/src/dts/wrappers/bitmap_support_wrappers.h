#pragma once
#include "types.h"

extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_THISCALL engineBitmap_ctor_impl(
        megusta::dts::EngineBitmapView* bitmap,std::uint32_t width,std::uint32_t height,
        char generateMipmaps,std::uint32_t pixelFormat);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL engineBitmap_readPng_impl(
        megusta::dts::EngineBitmapView* bitmap,void* file);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL engineBitmap_readBmp_impl(
        megusta::dts::EngineBitmapView* bitmap,void* opaqueFile);
extern "C" void MEGUSTA_DTS_THISCALL engineBitmap_resize_impl(
        megusta::dts::EngineBitmapView* bitmap,std::uint32_t width,std::uint32_t height,
        char generateMipmaps,std::uint32_t pixelFormat);
extern "C" void DTS_support_RGB888_to_RGB5551_impl(std::uint16_t* pixels,int pixelCount);
extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_CDECL
engineBitmap_expandPowerOfTwo_impl(megusta::dts::EngineBitmapView* bitmap);
