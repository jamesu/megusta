// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include "vector_storage.h"

namespace megusta { namespace dts {
struct EngineDisplaySize { int width,height; };
struct TSLastDetailMaterialState {
    // Named views of the same globals used by ordinary mesh rendering.
    int& textureMode;
    int& activeTextureUnit;
    int& baseTextureUnit;
    std::uint32_t& materialState;
    std::uint32_t& textureState;
};
struct DTSBillboardRenderState {
    float orientation[16]={};
    const float* postScale=nullptr;
    bool orientationRefreshPending=false;
    bool useStoredOrientation=false;
};
using DisplaySizeProvider = const int* (*)();
void bindDisplaySizeProvider(DisplaySizeProvider provider);
const EngineDisplaySize* queryDisplaySize();
extern TSLastDetailMaterialState lastDetailMaterialState;
extern DTSBillboardRenderState billboardRenderState;
void initializeLastDetailMaterials();
void resetLastDetailMaterials();
void forceFaceCamera();
} }

extern "C" {
void FUN_004d46e0(float* matrix);
megusta::dts::EngineDisplaySize* FUN_004d4860(void);
void FUN_004d48c0(void);
}
