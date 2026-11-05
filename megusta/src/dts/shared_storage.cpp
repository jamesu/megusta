#include "shared_storage.h"
#include <cstdlib>

namespace megusta { namespace dts {

DTSVectorStorage sharedMeshVerts;
DTSVectorStorage sharedMeshNorms;
DTSVectorStorage sharedEncodedNorms;
DTSVectorStorage sharedTextureVerts;
DTSVectorStorage sharedCopiedMeshData;
DTSVectorStorage sharedSkinTransforms;
DTSVectorStorage sharedSkinVertexIndices;
DTSVectorStorage sharedSkinBoneIndices;
DTSVectorStorage sharedSkinWeights;
DTSVectorStorage sharedSkinNodeIndices;
DTSVectorStorage sharedDecodedNormals;

namespace {
SharedStorageFree sharedFree = std::free;
std::uint8_t meshCleanupBits = 0;
std::uint8_t skinCleanupBits = 0;

void release(DTSVectorStorage& storage) {
    if (storage.data) {
        sharedFree(storage.data);
    }
}

void releaseOnce(DTSVectorStorage& storage, std::uint8_t& bits,
                 std::uint8_t bit) {
    if ((bits & bit) == 0) {
        bits = static_cast<std::uint8_t>(bits | bit);
        release(storage);
    }
}

void init(DTSVectorStorage& storage) {
    storage.count = 0;
    storage.data = nullptr;
    storage.capacity = 0;
}

} // namespace

void bindSharedStorageFree(SharedStorageFree freeFunction) {
    sharedFree = freeFunction ? freeFunction : std::free;
}

void freeSharedStorageAllocation(void* memory) {
    sharedFree(memory);
}

} }

using namespace megusta::dts;

#define DEFINE_MESH_STORAGE(prefix, object, bit) \
extern "C" void prefix##_initialize(void) { init(object); } \
extern "C" void prefix##_registerCleanup(void) { std::atexit(prefix##_cleanup); } \
extern "C" void prefix##_cleanup(void) { releaseOnce(object, meshCleanupBits, bit); }

#define DEFINE_SKIN_STORAGE(prefix, object, bit) \
extern "C" void prefix##_initialize(void) { init(object); } \
extern "C" void prefix##_registerCleanup(void) { std::atexit(prefix##_cleanup); } \
extern "C" void prefix##_cleanup(void) { releaseOnce(object, skinCleanupBits, bit); }

DEFINE_MESH_STORAGE(TSMesh_sharedVerts, sharedMeshVerts, 0x01)
DEFINE_MESH_STORAGE(TSMesh_sharedNorms, sharedMeshNorms, 0x02)
DEFINE_MESH_STORAGE(TSMesh_sharedEncodedNorms, sharedEncodedNorms, 0x04)
DEFINE_MESH_STORAGE(TSMesh_sharedTVerts, sharedTextureVerts, 0x08)
DEFINE_MESH_STORAGE(TSMesh_sharedDataCopied, sharedCopiedMeshData, 0x10)
DEFINE_MESH_STORAGE(TSSkinMesh_sharedTransforms, sharedSkinTransforms, 0x20)
DEFINE_MESH_STORAGE(TSSkinMesh_sharedVertexIndices, sharedSkinVertexIndices, 0x40)
DEFINE_MESH_STORAGE(TSSkinMesh_sharedBoneIndices, sharedSkinBoneIndices, 0x80)
DEFINE_SKIN_STORAGE(TSSkinMesh_sharedWeights, sharedSkinWeights, 0x01)
DEFINE_SKIN_STORAGE(TSSkinMesh_sharedNodeIndices, sharedSkinNodeIndices, 0x02)

extern "C" void TSMesh_decodedNormals_initialize(void) {
    init(sharedDecodedNormals);
}
extern "C" void TSMesh_decodedNormals_registerCleanup(void) {
    std::atexit(TSMesh_decodedNormals_cleanup);
}
extern "C" void TSMesh_decodedNormals_cleanup(void) {
    // The recovered routine has no one-shot bit guard for this buffer.
    release(sharedDecodedNormals);
}

#define DEFINE_STARTUP(address, initializer, registrar) \
extern "C" void DTS_registeredStartup_##address(void) { \
    initializer(); registrar(); \
}

DEFINE_STARTUP(004c0ce0, TSMesh_sharedVerts_initialize, TSMesh_sharedVerts_registerCleanup)
DEFINE_STARTUP(004c0d50, TSMesh_sharedNorms_initialize, TSMesh_sharedNorms_registerCleanup)
DEFINE_STARTUP(004c0dc0, TSMesh_sharedEncodedNorms_initialize, TSMesh_sharedEncodedNorms_registerCleanup)
DEFINE_STARTUP(004c0e30, TSMesh_sharedTVerts_initialize, TSMesh_sharedTVerts_registerCleanup)
DEFINE_STARTUP(004c0ea0, TSMesh_sharedDataCopied_initialize, TSMesh_sharedDataCopied_registerCleanup)
DEFINE_STARTUP(004c0f10, TSSkinMesh_sharedTransforms_initialize, TSSkinMesh_sharedTransforms_registerCleanup)
DEFINE_STARTUP(004c0f80, TSSkinMesh_sharedVertexIndices_initialize, TSSkinMesh_sharedVertexIndices_registerCleanup)
DEFINE_STARTUP(004c0ff0, TSSkinMesh_sharedBoneIndices_initialize, TSSkinMesh_sharedBoneIndices_registerCleanup)
DEFINE_STARTUP(004c1060, TSSkinMesh_sharedWeights_initialize, TSSkinMesh_sharedWeights_registerCleanup)
DEFINE_STARTUP(004c10d0, TSSkinMesh_sharedNodeIndices_initialize, TSSkinMesh_sharedNodeIndices_registerCleanup)
DEFINE_STARTUP(004c1140, TSMesh_decodedNormals_initialize, TSMesh_decodedNormals_registerCleanup)

#undef DEFINE_STARTUP
#undef DEFINE_SKIN_STORAGE
#undef DEFINE_MESH_STORAGE
