#include "matrix_wrappers.h"
using namespace megusta::dts;

extern "C" float* MEGUSTA_DTS_THISCALL DTS_QuatF_setMatrix_impl(
        const megusta::dts::DTSQuatF* self,float* matrix) {
    return self->setMatrix(matrix);
}

extern "C" void MEGUSTA_DTS_CDECL TSTransform_applyUniformScale_impl(
        float scale,float* matrix) {
    megusta::dts::applyUniformScale(scale,matrix);
}

extern "C" void MEGUSTA_DTS_CDECL TSTransform_applyAlignedScale_impl(
        const float* scale,float* matrix) {
    megusta::dts::applyAlignedScale(scale,matrix);
}

extern "C" void MEGUSTA_DTS_CDECL TSTransform_applyArbitraryScale_impl(
        const megusta::dts::DTSArbitraryScale* scale,float* matrix) {
    megusta::dts::applyArbitraryScale(scale,matrix);
}
