#include "graphics_state.h"
#include "mesh.h"
#include "shape.h"
#include "shadow_support.h"
#include "../engine/opengl_adapter.h"
#include <cstdlib>

namespace megusta { namespace dts {
TSLastDetailMaterialState lastDetailMaterialState{
    dtsTextureMode,dtsFogTextureUnit,dtsActiveTextureUnit,
    dtsCurrentMaterialFlags,dtsCurrentMaterialIndex};
DTSBillboardRenderState billboardRenderState;
namespace {
DisplaySizeProvider displaySizeProvider=nullptr;
EngineDisplaySize displaySize={0,0};
unsigned char cleanupRegistered=0;
unsigned char billboardCleanupRegistered=0;
}

void bindDisplaySizeProvider(DisplaySizeProvider provider) {
    displaySizeProvider=provider;
}

const EngineDisplaySize* queryDisplaySize() {
    if ((cleanupRegistered&1u)==0) {
        cleanupRegistered=static_cast<unsigned char>(cleanupRegistered|1u);
        std::atexit(FUN_004d48c0);
    }
    if (displaySizeProvider) {
        const int* dimensions=displaySizeProvider();
        displaySize.width=dimensions[0];
        displaySize.height=dimensions[1];
    } else {
        displaySize.width=0x280;
        displaySize.height=0x1e0;
    }
    return &displaySize;
}
void initializeLastDetailMaterials() {
    auto& state=lastDetailMaterialState;
    state.textureMode=8;
    state.activeTextureUnit=0;
    state.baseTextureUnit=0;
    state.materialState=0;
    state.textureState=0x10000000u;
    dtsRenderGL->enableClientState(0x1b);
    dtsRenderGL->enableClientState(0x1d);
    dtsRenderGL->enableClientState(0x1e);
    dtsRenderGL->texEnvi(0x48,0x49,0x56);
}
void resetLastDetailMaterials() {
    const auto& state=lastDetailMaterialState;
    dtsRenderGL->disableClientState(0x1d);
    dtsRenderGL->disableClientState(0x1e);
    dtsRenderGL->disableClientState(0x1b);
    dtsRenderGL->depthMask(true);
    dtsRenderGL->disable(0x2e);
    dtsRenderGL->disable(0x4a);
    if (state.textureMode==9) {
        dtsRenderGL->activeTexture(state.activeTextureUnit+0x8a);
        dtsRenderGL->texEnvi(0x48,0x49,0x65);
        dtsRenderGL->disable(0x4a);
        dtsRenderGL->activeTexture(state.baseTextureUnit+0x8a);
    }
}
void forceFaceCamera() {
    if ((billboardCleanupRegistered&4u)==0) {
        billboardCleanupRegistered=static_cast<unsigned char>(billboardCleanupRegistered|4u);
        std::atexit(DTS_billboardMatrix_cleanup_stub);
    }
    auto& state=billboardRenderState;
    if (state.orientationRefreshPending) {
        for (int i=0;i<16;++i) state.orientation[i]=(i%5==0) ? 1.0f : 0.0f;
        state.orientation[0]=-1.0f; state.orientation[4]=0.0f; state.orientation[8]=0.0f;
        state.orientation[1]=0.0f; state.orientation[5]=0.0f; state.orientation[9]=-1.0f;
        state.orientation[2]=0.0f; state.orientation[6]=1.0f; state.orientation[10]=0.0f;
        state.orientationRefreshPending=false;
    }
    float view[16];
    FUN_004d46e0(view);
    const float translation[4]={view[3],view[7],view[11],view[15]};
    float result[16];
    if (state.useStoredOrientation) {
        for (int i=0;i<16;++i) result[i]=state.orientation[i];
    } else {
        for (int i=0;i<16;++i) result[i]=(i%5==0) ? 1.0f : 0.0f;
    }
    result[3]=translation[0]; result[7]=translation[1]; result[11]=translation[2];
    result[15]=translation[3];
    transposeMatrix4(result);
    dtsRenderGL->loadMatrix(result);
    if (state.postScale)
        dtsRenderGL->scale(state.postScale[0],state.postScale[1],state.postScale[2]);
}
} }

extern "C" void FUN_004d46e0(float* matrix) {
    megusta::dts::dtsRenderGL->getFloatv(0x44,matrix);
    megusta::dts::transposeMatrix4(matrix);
}
extern "C" megusta::dts::EngineDisplaySize* FUN_004d4860(void) {
    return const_cast<megusta::dts::EngineDisplaySize*>(megusta::dts::queryDisplaySize());
}
extern "C" void FUN_004d48c0(void) {}
