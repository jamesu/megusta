// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_THISCALL TSSortedMesh_render_impl(
    megusta::dts::TSSortedMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials);
extern "C" const megusta::dts::TSMeshPoint* MEGUSTA_DTS_THISCALL TSMesh_getNormals_impl(
    megusta::dts::TSMesh* self,int vertexOffset);
extern "C" void MEGUSTA_DTS_THISCALL TSMesh_render_impl(
    megusta::dts::TSMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSMesh_hasDynamicMaterials_impl(
    megusta::dts::TSMesh* self,megusta::dts::TSMaterialList* materials);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSDecalMesh_hasDynamicMaterials_impl(
    megusta::dts::TSDecalMesh* self,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_initMaterials_impl(void);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_resetMaterials_impl(void);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_setMaterial_impl(
    std::uint32_t material,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_setFade_impl(float value);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_clearFade_impl(void);
extern "C" void MEGUSTA_DTS_THISCALL TSMesh_renderEnvironmentMap_impl(
    megusta::dts::TSMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_initEnvironmentMapMaterials_impl(void);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_resetEnvironmentMapMaterials_impl(void);
extern "C" void MEGUSTA_DTS_THISCALL TSMesh_renderFog_impl(
    megusta::dts::TSMesh* self,int frame,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_THISCALL TSSkinMesh_render_impl(
    megusta::dts::TSSkinMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_THISCALL TSDecalMesh_render_impl(
    megusta::dts::TSDecalMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_CDECL TSDecalMesh_initDecalMaterials_impl(void);
extern "C" void MEGUSTA_DTS_CDECL TSDecalMesh_resetDecalMaterials_impl(void);
extern "C" void MEGUSTA_DTS_CDECL DTS_setMeshColor_impl(float red,float green,float blue);
extern "C" void MEGUSTA_DTS_THISCALL TSMesh_renderShadow_impl(
    megusta::dts::TSMesh* self,int frame,const float* transform,int dimension,
    std::uint32_t* mask,megusta::dts::TSMaterialList* materials);
extern "C" void MEGUSTA_DTS_CDECL DTS_rasterizeTriangleStripShadow_impl(
    megusta::dts::TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
    const std::uint16_t* indices,const std::int32_t* coordinates,int dimension,std::uint32_t* mask);
extern "C" void MEGUSTA_DTS_CDECL DTS_rasterizeTriangleListShadow_impl(
    megusta::dts::TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
    const std::uint16_t* indices,const std::int32_t* coordinates,int dimension,std::uint32_t* mask);
extern "C" void engineRender_detectTextureExtensions_impl(void);
