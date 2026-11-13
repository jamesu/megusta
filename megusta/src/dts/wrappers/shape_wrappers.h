// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_FASTCALL TSShape_ctor_impl(megusta::dts::TSShape* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSShape_dtor_impl(megusta::dts::TSShape* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSShape_init_impl(megusta::dts::TSShape* self);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_read_impl(
        megusta::dts::TSShape* self,megusta::ResourceFile* file);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_importSequences_impl(
        megusta::dts::TSShape* self,megusta::ResourceFile* file);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_readName_impl(
        megusta::dts::TSShape* self,megusta::ResourceFile* file,char addIfMissing);
extern "C" char* MEGUSTA_DTS_THISCALL TSShape_getName_impl(
        megusta::dts::TSShape* self,int index);
extern "C" int MEGUSTA_DTS_THISCALL TSShape_findName_impl(
        megusta::dts::TSShape* self,char* name);
extern "C" int MEGUSTA_DTS_THISCALL TSShape_findNode_impl(
        megusta::dts::TSShape* self,int nameIndex);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSShape_checkSkip_impl(
        megusta::dts::TSShape* self,int meshIndex,int* objectCursor,
        std::uint32_t* decalCursor,std::uint32_t detailIndex);
extern "C" void MEGUSTA_DTS_THISCALL TSShape_setupBillboardDetails_impl(
        megusta::dts::TSShape* self,char* bitmapBaseName);
extern "C" void* MEGUSTA_DTS_THISCALL TSLastDetail_ctor_impl(
        megusta::dts::TSLastDetail* self,megusta::dts::TSShape* shape,
        std::uint32_t equatorSteps,int polarSteps,float polarAngle,char includePoles,
        int atlasIndex,int textureSize,char* bitmapBaseName);
extern "C" void MEGUSTA_DTS_FASTCALL TSLastDetail_dtor_impl(megusta::dts::TSLastDetail* self);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSLastDetail_loadBitmapViews_impl(
        megusta::dts::TSLastDetail* self,char* bitmapBaseName);
extern "C" void MEGUSTA_DTS_THISCALL TSLastDetail_renderNoFog_impl(
        megusta::dts::TSLastDetail* self,float alpha);
extern "C" void MEGUSTA_DTS_THISCALL TSLastDetail_renderFog_Combine_impl(
        megusta::dts::TSLastDetail* self,float alpha);
extern "C" void MEGUSTA_DTS_THISCALL TSLastDetail_render_impl(
        megusta::dts::TSLastDetail* self,float alpha,char fog);
extern "C" void MEGUSTA_DTS_CDECL TSTransform_interpolatePoint3F_impl(
        const float* first,const float* second,float position,float* output);
extern "C" void MEGUSTA_DTS_THISCALL DTS_Quat16_getQuatF_impl(
        const megusta::dts::DTSQuat16* self,float* output);
extern "C" void MEGUSTA_DTS_FASTCALL DTS_Quat16_set_impl(
        megusta::dts::DTSQuat16* self,const float* input);
extern "C" void MEGUSTA_DTS_FASTCALL TSShape_readIflMaterials_impl(
        megusta::dts::TSShape* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSShape_initMaterialList_impl(
        megusta::dts::TSShape* self);
extern "C" void MEGUSTA_DTS_THISCALL TSShape_rearrangeKeyframeData_impl(
        megusta::dts::TSShape* self,megusta::dts::TSShapeSequence* sequence,
        int firstKeyframe,unsigned char* rotations,unsigned char* translations,
        unsigned char* objectStates,unsigned char* decalStates,int rotationStride,
        int translationStride,int objectStride,int decalStride);
