#include "shape_instance_wrappers.h"
#include "../../engine/opengl_adapter.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_render_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials) {
    self->render(meshIndex,materials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSDecalObjectInstance_render_impl(
    megusta::dts::TSDecalObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials) {
    self->render(meshIndex,materials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_renderEnvironmentMap_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials) {
    self->renderEnvironmentMap(meshIndex,materials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_renderFog_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,megusta::dts::TSMaterialList* materials) {
    self->renderFog(meshIndex,materials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMeshObjectInstance_renderShadow_impl(
    megusta::dts::TSMeshObjectInstance* self,int meshIndex,const float* worldTransform,
    int dimension,std::uint32_t* mask,megusta::dts::TSMaterialList* materials) {
    self->renderShadow(meshIndex,worldTransform,dimension,mask,materials);
}

extern "C" megusta::dts::TSShapeInstance* MEGUSTA_DTS_THISCALL TSShapeInstance_ctor_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSShape* shape,int loadMaterials) {
    return new (self) megusta::dts::TSShapeInstance(shape,loadMaterials);
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_dtor_impl(
    megusta::dts::TSShapeInstance* self) {
    self->~TSShapeInstance();
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_handleMaskedPositionNode_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread,int node,int trackRank) {
    self->handleMaskedPositionNode(thread,node,trackRank);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateNodes_impl(
        megusta::dts::TSShapeInstance* self,int subShape) {
    self->animateNodeTransforms(subShape);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_handleBlendSequence_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread,
        int firstNode,int endNode) {
    self->handleBlendSequence(thread,firstNode,endNode);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateNodeSubtrees_impl(
        megusta::dts::TSShapeInstance* self,char markDirty) {
    self->animateNodeSubtrees(markDirty!=0);
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_animate_current_impl(
        megusta::dts::TSShapeInstance* self) {
    self->animate();
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animate_detail_impl(
        megusta::dts::TSShapeInstance* self,int detail) {
    self->animate(detail);
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_animateIfls_impl(
        megusta::dts::TSShapeInstance* self) {
    self->animateIfls();
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateVisibility_impl(
        megusta::dts::TSShapeInstance* self,int subShape) {
    self->animateVisibility(subShape);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateFrame_impl(
        megusta::dts::TSShapeInstance* self,int subShape) {
    self->animateFrame(subShape);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateMatFrame_impl(
        megusta::dts::TSShapeInstance* self,int subShape) {
    self->animateMatFrame(subShape);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_animateDecals_impl(
        megusta::dts::TSShapeInstance* self,std::uint32_t subShape) {
    self->animateDecals(subShape);
}

extern "C" megusta::dts::TSShapeInstance* MEGUSTA_DTS_THISCALL
TSShapeInstance_scalarDeletingDtor_impl(megusta::dts::TSShapeInstance* self,std::uint8_t flags) {
    TSShapeInstance_dtor_impl(self);
    if (flags&1u) ::operator delete(static_cast<void*>(self));
    return self;
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setStatics_impl(
    megusta::dts::TSShapeInstance* self,int detail,float intraDetail,const float* postScale) {
    self->setRenderStatics(detail,intraDetail,postScale);
}

extern "C" void MEGUSTA_DTS_CDECL TSShapeInstance_clearStatics_impl(void) {
    megusta::dts::dtsCurrentNodeTransforms=nullptr;
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setupTexturing_impl(
    megusta::dts::TSShapeInstance* self,int detail,float intraDetail) {
    self->setupTexturing(detail,intraDetail);
}

extern "C" bool MEGUSTA_DTS_CDECL TSShapeInstance_twoPassEnvironmentMap_impl(void) {
    return megusta::dts::dtsEnvironmentMode==3;
}

extern "C" bool MEGUSTA_DTS_CDECL TSShapeInstance_twoPassFog_impl(void) {
    return megusta::dts::dtsTextureMode==11;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_updateTransitions_impl(
    megusta::dts::TSShapeInstance* self) {
    self->updateTransitions();
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_renderEnvironmentMap_impl(
    megusta::dts::TSShapeInstance* self) {
    self->renderEnvironmentMap();
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShapeInstance_renderFog_impl(
    megusta::dts::TSShapeInstance* self) {
    self->renderFog();
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_render_detail_impl(
    megusta::dts::TSShapeInstance* self,int detail,float intraDetail,const float* postScale) {
    self->renderDetail(detail,intraDetail,postScale);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_render_current_impl(
    megusta::dts::TSShapeInstance* self,const float* postScale) {
    self->renderCurrent(postScale);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_renderShadow_impl(
    megusta::dts::TSShapeInstance* self,int detail,const float* worldTransform,
    int dimension,std::uint32_t* mask) {
    self->renderShadow(detail,worldTransform,dimension,mask);
}

extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_THISCALL TSShapeInstance_snapshot_impl(
        megusta::dts::TSShapeInstance* self,int width,int height,char generateMipmaps,
        float* transform,char allowUpscale) {
    return self->snapshot(width,height,generateMipmaps,transform,allowUpscale);
}

extern "C" megusta::dts::EngineBitmapView* MEGUSTA_DTS_CDECL TSShapeInstance_snapshot_static_impl(
        megusta::dts::TSShape* shape,int width,int height,char generateMipmaps,float* transform,
        int detail,float intraDetail,char allowUpscale) {
    return megusta::dts::snapshotShapeStatic(shape,width,height,generateMipmaps,
        transform,detail,intraDetail,allowUpscale);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_buildInstanceData_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSShape* shape,int loadMaterials) {
    (void)self->buildInstanceData(shape,loadMaterials);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setMaterialList_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSMaterialList* materials) {
    self->setMaterialList(materials);
}

extern "C" megusta::dts::TSThread* MEGUSTA_DTS_FASTCALL TSShapeInstance_addThread_impl(
        megusta::dts::TSShapeInstance* self) {
    return self->addThread();
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_destroyThread_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread) {
    self->destroyThread(thread);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_setSequence_impl(
        megusta::dts::TSShapeInstance* self,megusta::dts::TSThread* thread,
        int sequence,float position) {
    self->setSequence(thread,sequence,position);
}

extern "C" void MEGUSTA_DTS_THISCALL TSShapeInstance_advanceTime_all_impl(
        megusta::dts::TSShapeInstance* self,float delta) {
    self->advanceTime(delta);
}
