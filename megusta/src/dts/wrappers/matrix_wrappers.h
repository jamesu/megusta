#pragma once
#include "types.h"

extern "C" float* MEGUSTA_DTS_THISCALL DTS_QuatF_setMatrix_impl(
        const megusta::dts::DTSQuatF* self,float* matrix);
extern "C" void MEGUSTA_DTS_CDECL TSTransform_applyUniformScale_impl(
        float scale,float* matrix);
extern "C" void MEGUSTA_DTS_CDECL TSTransform_applyAlignedScale_impl(
        const float* scale,float* matrix);
extern "C" void MEGUSTA_DTS_CDECL TSTransform_applyArbitraryScale_impl(
        const megusta::dts::DTSArbitraryScale* scale,float* matrix);
