// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_render_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_THISCALL TSDecalObjectInstance_render_impl(
    megusta::dts::TSDecalObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_renderEnvironmentMap_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_renderFog_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_renderShadow_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,const float* worldTransform,
    int dimension,std::uint32_t* mask,megusta::dts::TSMaterialList* materials);
extern "C" megusta::dts::TSShapeInstance* MEGUSTA_DTS_THISCALL TSShapeInstance_ctor_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSShape* shape,int loadMaterials);
extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_dtor_impl(
    megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_handleMaskedPositionNode_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread,int node,int trackRank);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateNodes_impl(
        megusta::dts::TSShapeInstance* self,int subShape);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_handleBlendSequence_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread,
        int firstNode,int endNode);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateNodeSubtrees_impl(
        megusta::dts::TSShapeInstance* self,char markDirty);
extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_animate_current_impl(
        megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animate_detail_impl(
        megusta::dts::TSShapeInstance* self,int detail);
extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_animateIfls_impl(
        megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateVisibility_impl(
        megusta::dts::TSShapeInstance* self,int subShape);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateFrame_impl(
        megusta::dts::TSShapeInstance* self,int subShape);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateMatFrame_impl(
        megusta::dts::TSShapeInstance* self,int subShape);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateDecals_impl(
        megusta::dts::TSShapeInstance* self,std::uint32_t subShape);
extern "C" megusta::dts::TSShapeInstance* MEGUSTA_DTS_THISCALL
TSShapeInstance_scalarDeletingDtor_impl(megusta::dts::TSShapeInstance* self,std::uint8_t flags);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setStatics_impl(
    megusta::dts::TSShapeInstance* self,int detail,float intraDetail,const float* postScale);
extern "C" void MEGUSTA_DTS_CDECL TSShapeInstance_clearStatics_impl(void);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setupTexturing_impl(
    megusta::dts::TSShapeInstance* self,int detail,float intraDetail);
extern "C" bool MEGUSTA_DTS_CDECL TSShapeInstance_twoPassEnvironmentMap_impl(void);
extern "C" bool MEGUSTA_DTS_CDECL TSShapeInstance_twoPassFog_impl(void);
extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_updateTransitions_impl(
    megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_renderEnvironmentMap_impl(
    megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_renderFog_impl(
    megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_render_detail_impl(
    megusta::dts::TSShapeInstance* self,int detail,float intraDetail,const float* postScale);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_render_current_impl(
    megusta::dts::TSShapeInstance* self,const float* postScale);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_renderShadow_impl(
    megusta::dts::TSShapeInstance* self,int detail,const float* worldTransform,
    int dimension,std::uint32_t* mask);
extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_THISCALL TSShapeInstance_snapshot_impl(
    megusta::dts::TSShapeInstance* self,int width,int height,char generateMipmaps,
    float* transform,char allowUpscale);
extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_CDECL TSShapeInstance_snapshot_static_impl(
    megusta::dts::TSShape* shape,int width,int height,char generateMipmaps,float* transform,
    int detail,float intraDetail,char allowUpscale);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_buildInstanceData_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSShape* shape,int loadMaterials);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setMaterialList_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSMaterialList* materials);
extern "C" megusta::dts::TSThread* MEGUSTA_DTS_FASTCALL TSShapeInstance_addThread_impl(
        megusta::dts::TSShapeInstance* self);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_destroyThread_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setSequence_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread,
        int sequence,float position);
extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_advanceTime_all_impl(
        megusta::dts::TSShapeInstance* self,float delta);
