// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
namespace megusta {
class OpenGLAdapter;
struct GLSetupState {
    int swapInterval;
    int lastHintTarget,lastHintMode;
    void (*hintHook)(unsigned,unsigned)=nullptr;
    void* (*resolveProcedure)(const char*)=nullptr;
};
struct PalettedTextureState {
    int colorBits;
    int supported=-1,maximumTextureSize=-1;
    void* colorTableProcedure=nullptr;
    void* (*resolveProcedure)(const char*)=nullptr;
};
void initializeExtensionsAndSwapInterval(OpenGLAdapter&,GLSetupState&,int savedArgumentA,int savedArgumentB);
bool uploadPalettedMipChain(PalettedTextureState&,int width,int height,int levels,
    const std::uint8_t* pixels,const std::uint8_t* rgbaPalette,int firstMip);
}
