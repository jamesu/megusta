#include "shared_storage.h"
#include "shadow_support.h"
#include "scratch_storage.h"
#include "scratch_release.h"
namespace megusta::dts {
void releaseMeshScratchBuffers() {
    sharedDecodedNormals.count=0; sharedDecodedNormals.resize(0,12);
    shadowProjectionStorage.count=0; shadowProjectionStorage.resize(0,8);
}
void releaseLegacyScratchBuffers() {
    scratchShapeRead0.count=0; scratchShapeRead0.resize(0,4);
    scratchShapeRead1.count=0; scratchShapeRead1.resize(0,12);
}
void releaseShapeReadScratchBuffers() { scratchShapeRead2.count=0; scratchShapeRead2.resize(0,64); }
void releaseInstanceScratchBuffers() {
    struct Entry { DTSVectorStorage* storage; unsigned width; };
    const Entry entries[]={
        {&sharedMeshVerts,4},{&sharedMeshNorms,4},{&sharedEncodedNorms,1},{&sharedTextureVerts,4},
        {&sharedCopiedMeshData,4},{&sharedSkinTransforms,4},{&sharedSkinVertexIndices,4},
        {&sharedSkinBoneIndices,4},{&sharedSkinWeights,4},{&sharedSkinNodeIndices,4},
        {&scratchNodeValues,16},{&scratchNodeTranslations,12},{&scratchUniformScales,4},
        {&scratchAlignedScales,12},{&scratchArbitraryScales,28},{&scratchAuxiliary0,4},
        {&scratchAuxiliary1,4},{&scratchScaleThreads,4}};
    for (const auto& entry:entries) { entry.storage->count=0; entry.storage->resize(0,entry.width); }
}
}



