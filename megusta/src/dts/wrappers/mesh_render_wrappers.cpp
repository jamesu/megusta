#include "mesh_render_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_THISCALL TSSortedMesh_render_impl(
    megusta::dts::TSSortedMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials) {
    self->render(frame,materialFrame,materials);
}

extern "C" const megusta::dts::TSMeshPoint* MEGUSTA_DTS_THISCALL TSMesh_getNormals_impl(
    megusta::dts::TSMesh* self,int vertexOffset) {
    return self->getNormals(vertexOffset);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMesh_render_impl(
    megusta::dts::TSMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials) {
    self->render(frame,materialFrame,materials);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSMesh_hasDynamicMaterials_impl(
    megusta::dts::TSMesh* self,megusta::dts::TSMaterialList* materials) {
    return self->hasDynamicMaterials(materials);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSDecalMesh_hasDynamicMaterials_impl(
    megusta::dts::TSDecalMesh* self,megusta::dts::TSMaterialList* materials) {
    return self->hasDynamicMaterials(materials);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_initMaterials_impl(void) {
    megusta::dts::TSMesh::initMaterials();
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_resetMaterials_impl(void) {
    megusta::dts::TSMesh::resetMaterials();
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_setMaterial_impl(
    std::uint32_t material,megusta::dts::TSMaterialList* materials) {
    megusta::dts::TSMesh::setMaterial(material,materials);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_setFade_impl(float value) {
    megusta::dts::TSMesh::setFade(value);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_clearFade_impl(void) {
    megusta::dts::TSMesh::clearFade();
}

extern "C" void MEGUSTA_DTS_THISCALL TSMesh_renderEnvironmentMap_impl(
    megusta::dts::TSMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials) {
    self->renderEnvironmentMap(frame,materialFrame,materials);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_initEnvironmentMapMaterials_impl(void) {
    megusta::dts::TSMesh::initEnvironmentMapMaterials();
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_resetEnvironmentMapMaterials_impl(void) {
    megusta::dts::TSMesh::resetEnvironmentMapMaterials();
}

extern "C" void MEGUSTA_DTS_THISCALL TSMesh_renderFog_impl(
    megusta::dts::TSMesh* self,int frame,megusta::dts::TSMaterialList* materials) {
    self->renderFog(frame,materials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSSkinMesh_render_impl(
    megusta::dts::TSSkinMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials) {
    self->render(frame,materialFrame,materials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSDecalMesh_render_impl(
    megusta::dts::TSDecalMesh* self,int frame,int materialFrame,
    megusta::dts::TSMaterialList* materials) {
    self->render(frame,materialFrame,materials);
}

extern "C" void MEGUSTA_DTS_CDECL TSDecalMesh_initDecalMaterials_impl(void) {
    megusta::dts::TSDecalMesh::initDecalMaterials();
}

extern "C" void MEGUSTA_DTS_CDECL TSDecalMesh_resetDecalMaterials_impl(void) {
    megusta::dts::TSDecalMesh::resetDecalMaterials();
}

extern "C" void MEGUSTA_DTS_CDECL DTS_setMeshColor_impl(float red,float green,float blue) {
    megusta::dts::setMeshColor(red,green,blue);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMesh_renderShadow_impl(
    megusta::dts::TSMesh* self,int frame,const float* transform,int dimension,
    std::uint32_t* mask,megusta::dts::TSMaterialList* materials) {
    self->renderShadow(frame,transform,dimension,mask,materials);
}

extern "C" void MEGUSTA_DTS_CDECL DTS_rasterizeTriangleStripShadow_impl(
    megusta::dts::TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
    const std::uint16_t* indices,const std::int32_t* coordinates,int dimension,std::uint32_t* mask) {
    megusta::dts::rasterizeTriangleStripShadows(
        primitives,primitiveCount,primitiveStride,indices,coordinates,dimension,mask);
}

extern "C" void MEGUSTA_DTS_CDECL DTS_rasterizeTriangleListShadow_impl(
    megusta::dts::TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
    const std::uint16_t* indices,const std::int32_t* coordinates,int dimension,std::uint32_t* mask) {
    megusta::dts::rasterizeTriangleListShadows(
        primitives,primitiveCount,primitiveStride,indices,coordinates,dimension,mask);
}

extern "C" void engineRender_detectTextureExtensions_impl(void) {
    megusta::dts::detectTextureExtensions();
}
