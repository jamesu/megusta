#include "mesh_wrappers.h"
#include <cstring>
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_CDECL TSMesh_initializeNormalDecodeTable_impl(void) {
    initializeNormalDecodeTable();
}

extern "C" int MEGUSTA_DTS_FASTCALL TSMesh_getNumPolys_impl(TSMesh* self) {
    return self->getNumPolys();
}

extern "C" void MEGUSTA_DTS_FASTCALL TSMesh_computeBounds_impl(TSMesh* self) {
    // The low three bits carry the mesh subtype; higher bits describe render
    // state and encoded data, and do not change the virtual bounds operation.
    if ((static_cast<std::uint32_t>(self->meshType)&7u)==1u)
        static_cast<TSSkinMesh*>(self)->computeBounds();
    else self->computeBounds();
}

extern "C" void MEGUSTA_DTS_THISCALL TSMesh_computeBounds_frame_impl(
        TSMesh* self,const float* matrix,float* bounds,int frame,float* center,float* radius) {
    self->computeBounds(matrix,bounds,frame,center,radius);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_computeBounds_vertices_impl(
        const float* vertices,int count,const float* matrix,float* bounds,
        float* center,float* radius) {
    computeVertexBounds(reinterpret_cast<const TSMeshPoint*>(vertices),count,matrix,
                        bounds,center,radius);
}

extern "C" void MEGUSTA_DTS_THISCALL TSSkinMesh_computeBounds_impl(
        TSSkinMesh* self,const float* matrix,float* bounds,int frame,
        float* center,float* radius) {
    self->computeBounds(matrix,bounds,frame,center,radius);
}

extern "C" void MEGUSTA_DTS_CDECL DTS_unwindStrip_impl(
        const std::uint16_t* indices,int count,
        megusta::dts::DTSUnwindStripVector* output) {
    megusta::dts::unwindTriangleStrip(indices,count,*output);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_convertToTris_impl(
        const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
        int* outputIndexCount,megusta::dts::TSDrawPrimitive* outputPrimitives,
        std::uint16_t* outputIndices) {
    megusta::dts::convertToTris(ranges,materials,indices,primitiveCount,
        outputPrimitiveCount,outputIndexCount,outputPrimitives,outputIndices);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_convertToSingleStrip_impl(
        const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
        std::uint32_t* outputIndexCount,megusta::dts::TSDrawPrimitive* outputPrimitives,
        std::uint16_t* outputIndices) {
    megusta::dts::convertToSingleStrip(ranges,materials,indices,primitiveCount,
        outputPrimitiveCount,outputIndexCount,outputPrimitives,outputIndices);
}

extern "C" void MEGUSTA_DTS_CDECL TSMesh_leaveAsMultipleStrips_impl(
        const std::uint16_t (*ranges)[2],const std::uint32_t* materials,
        const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
        int* outputIndexCount,megusta::dts::TSDrawPrimitive* outputPrimitives,
        std::uint16_t* outputIndices) {
    megusta::dts::leaveAsMultipleStrips(ranges,materials,indices,primitiveCount,
        outputPrimitiveCount,outputIndexCount,outputPrimitives,outputIndices);
}

extern "C" int MEGUSTA_DTS_CDECL DTS_reduceSkinInfluences_impl(
        const std::int32_t* vertexIndices,const std::int32_t* boneIndices,
        const float* weights,std::int32_t* outputVertices,std::int32_t* outputBones,
        float* outputWeights,int influenceCount) {
    return megusta::dts::reduceSkinInfluences(vertexIndices,boneIndices,weights,
        outputVertices,outputBones,outputWeights,influenceCount);
}

extern "C" void MEGUSTA_DTS_CDECL DTS_getSharedSkinInfluences_impl(
        int initialVertexCount,int parentMesh,int* influenceCount,
        const std::int32_t* const* vertexData,const std::int32_t* const* boneData,
        const float* const* weightData,std::int32_t** outputVertices,
        std::int32_t** outputBones,float** outputWeights,char skip) {
    megusta::dts::getSharedSkinInfluences(initialVertexCount,parentMesh,influenceCount,
        vertexData,boneData,weightData,outputVertices,outputBones,outputWeights,skip);
}

extern "C" void* MEGUSTA_DTS_CDECL TSMesh_assembleMesh_impl(int meshType,std::uint8_t skip) {
    return megusta::dts::TSMesh::assembleMesh(
        static_cast<megusta::dts::DTSMeshKind>(meshType),skip);
}

extern "C" void* MEGUSTA_DTS_CDECL TSMesh_getSharedData32_impl(
        int parent,int count,void** parentData,char skip) {
    return megusta::dts::sharedMeshData32(parent,count,parentData,skip);
}

extern "C" void* MEGUSTA_DTS_CDECL TSMesh_getSharedData8_impl(
        int parent,int count,void** parentData,char skip) {
    return megusta::dts::sharedMeshData8(parent,count,parentData,skip);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMesh_assemble_impl(
        megusta::dts::TSMesh* self,char skip) {
    self->assemble(skip);
}

extern "C" void MEGUSTA_DTS_THISCALL TSSkinMesh_assemble_impl(
        megusta::dts::TSSkinMesh* self,int skip) {
    self->assemble(skip);
}

extern "C" void MEGUSTA_DTS_THISCALL TSSortedMesh_assemble_impl(
        megusta::dts::TSSortedMesh* self,char skip) {
    self->assemble(skip);
}

extern "C" void MEGUSTA_DTS_FASTCALL TSDecalMesh_assemble_impl(
        megusta::dts::TSDecalMesh* self) {
    self->assemble();
}
