#include "texture_manager_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_CDECL engineTexture_destroyEntry_impl(
        megusta::dts::EngineTextureEntry* entry) {
    megusta::dts::destroyTextureEntry(entry);
}

extern "C" std::uint32_t MEGUSTA_DTS_CDECL engineTexture_uploadMipChain_impl(
        megusta::dts::EngineBitmapView* bitmap,char clamp,std::uint32_t firstMip) {
    return megusta::dts::uploadBitmapMipChain(bitmap,clamp!=0,firstMip);
}

extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_createDynamicEntry_impl(const char* filename,
        megusta::dts::EngineBitmapView* bitmap,char clamp) {
    return megusta::dts::createDynamicTextureEntry(filename,bitmap,clamp!=0);
}

extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_createOrReplaceEntry_impl(const std::uint8_t* filename,
        megusta::dts::EngineBitmapView* bitmap,int kind,char clamp) {
    return megusta::dts::createOrReplaceTextureEntry(
        reinterpret_cast<const char*>(filename),bitmap,kind,clamp!=0);
}

extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_loadByName_impl(const std::uint8_t* filename,int kind,char clamp) {
    return megusta::dts::loadTextureByName(
        reinterpret_cast<const char*>(filename),kind,clamp!=0);
}

extern "C" megusta::dts::EngineTextureEntry* MEGUSTA_DTS_CDECL
engineTexture_findCached_impl(const std::uint8_t* filename,int kind) {
    return megusta::dts::findTextureEntry(
        reinterpret_cast<const char*>(filename),kind);
}

extern "C" void MEGUSTA_DTS_CDECL engineTexture_updatePixels_impl(
        megusta::dts::EngineTextureEntry* entry) {
    megusta::dts::updateTexturePixels(entry);
}

extern "C" void MEGUSTA_DTS_CDECL engineTexture_mapPixelFormat_impl(
        megusta::dts::EngineBitmapView* bitmap,int* format,int* internalFormat,int* pixelType) {
    (void)megusta::dts::mapTexturePixelFormat(bitmap,format,internalFormat,pixelType);
}

extern "C" void MEGUSTA_DTS_FASTCALL engineTextureHandle_retain_impl(
        megusta::dts::EngineTextureHandle* self) {
    if (self) megusta::dts::retainTextureEntry(self->entry);
}

extern "C" void MEGUSTA_DTS_FASTCALL engineTextureHandle_release_impl(
        megusta::dts::EngineTextureHandle* self) {
    if (self) self->entry=megusta::dts::releaseTextureEntry(self->entry);
}

extern "C" void MEGUSTA_DTS_FASTCALL engineTextureHandle_refresh_impl(
        megusta::dts::EngineTextureHandle* self) {
    if (self) megusta::dts::updateTexturePixels(self->entry);
}
