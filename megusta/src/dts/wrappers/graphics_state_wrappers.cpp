#include "graphics_state_wrappers.h"
using namespace megusta::dts;

extern "C" void TSLastDetail_initMaterials_impl(void) {
    megusta::dts::initializeLastDetailMaterials();
}

extern "C" void TSLastDetail_resetMaterials_impl(void) {
    megusta::dts::resetLastDetailMaterials();
}

extern "C" void MEGUSTA_DTS_CDECL DTS_forceFaceCamera_impl(void) {
    megusta::dts::forceFaceCamera();
}
