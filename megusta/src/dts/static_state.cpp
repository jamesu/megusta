#include "static_state.h"

namespace megusta { namespace dts {
std::uint32_t materialRenderStateWords[8]={};
std::uint32_t billboardBasisStateWords[12]={};
} }

extern "C" void DTS_registeredStartup_004d4700(void) {}
extern "C" void DTS_staticInitializer_004d4710(void) {}

namespace {
void initializeMaterialRenderState() {
    using megusta::dts::materialRenderStateWords;
    materialRenderStateWords[0]=0;
    materialRenderStateWords[1]=0x3f800000;
    materialRenderStateWords[2]=0x3f800000;
    materialRenderStateWords[3]=0x3f800000;
    materialRenderStateWords[4]=0x3f800000;
    materialRenderStateWords[5]=0;
    materialRenderStateWords[6]=0;
    materialRenderStateWords[7]=0;
}
void initializeBillboardBasisState() {
    using megusta::dts::billboardBasisStateWords;
    billboardBasisStateWords[0]=0; billboardBasisStateWords[1]=0xbf800000;
    billboardBasisStateWords[2]=0;
    billboardBasisStateWords[3]=0; billboardBasisStateWords[4]=0xbf800000;
    billboardBasisStateWords[5]=0;
    billboardBasisStateWords[6]=0; billboardBasisStateWords[7]=0xbf800000;
    billboardBasisStateWords[8]=0;
    billboardBasisStateWords[9]=0; billboardBasisStateWords[10]=0xbf800000;
    billboardBasisStateWords[11]=0;
}
}

extern "C" void DTS_registeredStartup_004d5db0(void) { initializeMaterialRenderState(); }
extern "C" void DTS_staticInitializer_004d5dc0(void) { initializeMaterialRenderState(); }
extern "C" void DTS_registeredStartup_004d5e20(void) { initializeBillboardBasisState(); }
extern "C" void DTS_staticInitializer_004d5e30(void) { initializeBillboardBasisState(); }
