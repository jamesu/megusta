// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_CDECL engineTexture_destroyEntry_impl(
        megusta::dts::EngineTextureEntry* entry);
extern "C" std::uint32_t MEGUSTA_DTS_CDECL engineTexture_uploadMipChain_impl(
        megusta::dts::EngineBitmapView* bitmap,char clamp,std::uint32_t firstMip);
extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_createDynamicEntry_impl(const char* filename,
        megusta::dts::EngineBitmapView* bitmap,char clamp);
extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_createOrReplaceEntry_impl(const std::uint8_t* filename,
        megusta::dts::EngineBitmapView* bitmap,int kind,char clamp);
extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_loadByName_impl(const std::uint8_t* filename,int kind,char clamp);
extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_findCached_impl(const std::uint8_t* filename,int kind);
extern "C" void MEGUSTA_DTS_CDECL engineTexture_updatePixels_impl(
        megusta::dts::EngineTextureEntry* entry);
extern "C" void MEGUSTA_DTS_CDECL engineTexture_mapPixelFormat_impl(
        megusta::dts::EngineBitmapView* bitmap,int* format,int* internalFormat,int* pixelType);
extern "C" void MEGUSTA_DTS_FASTCALL engineTextureHandle_retain_impl(
        megusta::dts::EngineTextureHandle* self);
extern "C" void MEGUSTA_DTS_FASTCALL engineTextureHandle_release_impl(
        megusta::dts::EngineTextureHandle* self);
extern "C" void MEGUSTA_DTS_FASTCALL engineTextureHandle_refresh_impl(
        megusta::dts::EngineTextureHandle* self);
