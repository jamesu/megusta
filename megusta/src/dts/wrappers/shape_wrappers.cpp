#include "shape_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_FASTCALL TSShape_ctor_impl(megusta::dts::TSShape* self) {
    // Default initialization preserves the saved constructor's unwritten
    // scalar fields while constructing the normalized C++ containers.
    new (self) megusta::dts::TSShape;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShape_dtor_impl(megusta::dts::TSShape* self) {
    self->destroy();
    self->~TSShape();
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShape_init_impl(megusta::dts::TSShape* self) {
    self->init();
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_read_impl(
        megusta::dts::TSShape* self,megusta::ResourceFile* file) {
    // Keep the recovered two-argument thiscall ABI. Assembly mode comes from
    // the same process-wide switches consumed by TSMesh_assemble_impl; the
    // game assets are v16, so the encoded-normal switch only affects v22 data.
    return self->read(*file,megusta::dts::legacyMaterialPrefix,
        megusta::dts::dtsUseEncodedNormals,megusta::dts::shapeSkipLoadDetails,
        megusta::dts::dtsConvertTriangles,megusta::dts::minimumStripTriangles) ? 1u : 0u;
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_importSequences_impl(
        megusta::dts::TSShape* self,megusta::ResourceFile* file) {
    return self->importSequences(*file,megusta::dts::dtsShapeInitializeAfterRead) ? 1u : 0u;
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_readName_impl(
        megusta::dts::TSShape* self,megusta::ResourceFile* file,char addIfMissing) {
    return static_cast<std::uint32_t>(self->readName(file,addIfMissing));
}

extern "C" char* MEGUSTA_DTS_THISCALL TSShape_getName_impl(
        megusta::dts::TSShape* self,int index) {
    return self->getName(index);
}

extern "C" int MEGUSTA_DTS_THISCALL TSShape_findName_impl(
        megusta::dts::TSShape* self,char* name) {
    return self->findName(name);
}

extern "C" int MEGUSTA_DTS_THISCALL TSShape_findNode_impl(
        megusta::dts::TSShape* self,int nameIndex) {
    return self->findNode(nameIndex);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_checkSkip_impl(
        megusta::dts::TSShape* self,int meshIndex,int* objectCursor,
        std::uint32_t* decalCursor,std::uint32_t detailIndex) {
    return self->checkSkip(meshIndex,objectCursor,decalCursor,detailIndex) ? 1u : 0u;
}

extern "C" void MEGUSTA_DTS_THISCALL TSShape_setupBillboardDetails_impl(
        megusta::dts::TSShape* self,char* bitmapBaseName) {
    self->setupBillboardDetails(bitmapBaseName);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSLastDetail_ctor_impl(
        megusta::dts::TSLastDetail* self,megusta::dts::TSShape* shape,
        std::uint32_t equatorSteps,int polarSteps,float polarAngle,char includePoles,
        int atlasIndex,int textureSize,char* bitmapBaseName) {
    new (self) megusta::dts::TSLastDetail();
    self->construct(shape,equatorSteps,static_cast<std::uint32_t>(polarSteps),polarAngle,
                    includePoles,atlasIndex,textureSize,bitmapBaseName);
    return self;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSLastDetail_dtor_impl(megusta::dts::TSLastDetail* self) {
    self->~TSLastDetail();
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSLastDetail_loadBitmapViews_impl(
        megusta::dts::TSLastDetail* self,char* bitmapBaseName) {
    return self->loadBitmapViews(bitmapBaseName) ? 1u : 0u;
}

extern "C" void MEGUSTA_DTS_THISCALL TSLastDetail_renderNoFog_impl(
        megusta::dts::TSLastDetail* self,float alpha) {
    self->renderNoFog(alpha);
}

extern "C" void MEGUSTA_DTS_THISCALL TSLastDetail_renderFog_Combine_impl(
        megusta::dts::TSLastDetail* self,float alpha) {
    self->renderFogCombine(alpha);
}

extern "C" void MEGUSTA_DTS_THISCALL TSLastDetail_render_impl(
        megusta::dts::TSLastDetail* self,float alpha,char fog) {
    self->render(alpha,fog!=0);
}

extern "C" void MEGUSTA_DTS_CDECL TSTransform_interpolatePoint3F_impl(
        const float* first,const float* second,float position,float* output) {
    megusta::dts::interpolatePoint3F(first,second,position,output);
}

extern "C" void MEGUSTA_DTS_THISCALL DTS_Quat16_getQuatF_impl(
        const megusta::dts::DTSQuat16* self,float* output) {
    self->getQuatF(output);
}

extern "C" void MEGUSTA_DTS_FASTCALL DTS_Quat16_set_impl(
        megusta::dts::DTSQuat16* self,const float* input) {
    self->set(input);
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShape_readIflMaterials_impl(
        megusta::dts::TSShape* self) {
    self->readIflMaterials();
}

extern "C" void MEGUSTA_DTS_FASTCALL TSShape_initMaterialList_impl(
        megusta::dts::TSShape* self) {
    self->initMaterialList();
}

extern "C" void MEGUSTA_DTS_THISCALL TSShape_rearrangeKeyframeData_impl(
        megusta::dts::TSShape* self,megusta::dts::TSShapeSequence* sequence,
        int firstKeyframe,unsigned char* rotations,unsigned char* translations,
        unsigned char* objectStates,unsigned char* decalStates,int rotationStride,
        int translationStride,int objectStride,int decalStride) {
    self->rearrangeKeyframeData(sequence,firstKeyframe,rotations,translations,
        objectStates,decalStates,rotationStride,translationStride,objectStride,decalStride);
}
