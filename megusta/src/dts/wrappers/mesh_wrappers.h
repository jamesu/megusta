#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_CDECL TSMesh_initializeNormalDecodeTable_impl(void);
extern "C" int MEGUSTA_DTS_FASTCALL TSMesh_getNumPolys_impl(megusta::dts::TSMesh* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSMesh_computeBounds_impl(megusta::dts::TSMesh* self);
extern "C" void MEGUSTA_DTS_THISCALL TSMesh_computeBounds_frame_impl(
        megusta::dts::TSMesh* self,const float* matrix,float* bounds,int frame,float* center,float* radius);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_computeBounds_vertices_impl(
        const float* vertices,int count,const float* matrix,float* bounds,
        float* center,float* radius);
extern "C" void MEGUSTA_DTS_THISCALL TSSkinMesh_computeBounds_impl(
        megusta::dts::TSSkinMesh* self,const float* matrix,float* bounds,int frame,
        float* center,float* radius);
extern "C" void MEGUSTA_DTS_CDECL DTS_unwindStrip_impl(
        const std::uint16_t* indices,int count,
        megusta::dts::DTSUnwindStripVector* output);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_convertToTris_impl(
        const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
        int* outputIndexCount,megusta::dts::TSDrawPrimitive* outputPrimitives,
        std::uint16_t* outputIndices);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_convertToSingleStrip_impl(
        const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
        std::uint32_t* outputIndexCount,megusta::dts::TSDrawPrimitive* outputPrimitives,
        std::uint16_t* outputIndices);
extern "C" void MEGUSTA_DTS_CDECL TSMesh_leaveAsMultipleStrips_impl(
        const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
        int* outputIndexCount,megusta::dts::TSDrawPrimitive* outputPrimitives,
        std::uint16_t* outputIndices);
extern "C" int MEGUSTA_DTS_CDECL DTS_reduceSkinInfluences_impl(
        const std::int32_t* vertexIndices,const std::int32_t* boneIndices,
        const float* weights,std::int32_t* outputVertices,std::int32_t* outputBones,
        float* outputWeights,int influenceCount);
extern "C" void MEGUSTA_DTS_CDECL DTS_getSharedSkinInfluences_impl(
        int initialVertexCount,int parentMesh,int* influenceCount,
        const std::int32_t* const* vertexData,const std::int32_t* const* boneData,
        const float* const* weightData,std::int32_t** outputVertices,
        std::int32_t** outputBones,float** outputWeights,char skip);
extern "C" void* MEGUSTA_DTS_CDECL TSMesh_assembleMesh_impl(int meshType,std::uint8_t skip);
extern "C" void* MEGUSTA_DTS_CDECL TSMesh_getSharedData32_impl(
        int parent,int count,void** parentData,char skip);
extern "C" void* MEGUSTA_DTS_CDECL TSMesh_getSharedData8_impl(
        int parent,int count,void** parentData,char skip);
extern "C" void MEGUSTA_DTS_THISCALL TSMesh_assemble_impl(
        megusta::dts::TSMesh* self,char skip);
extern "C" void MEGUSTA_DTS_THISCALL TSSkinMesh_assemble_impl(
        megusta::dts::TSSkinMesh* self,int skip);
extern "C" void MEGUSTA_DTS_THISCALL TSSortedMesh_assemble_impl(
        megusta::dts::TSSortedMesh* self,char skip);
extern "C" void MEGUSTA_DTS_FASTCALL TSDecalMesh_assemble_impl(
        megusta::dts::TSDecalMesh* self);
