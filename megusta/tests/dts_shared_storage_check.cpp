#include "../src/dts/shared_storage.h"
#include <cassert>
#include <cstdlib>

namespace {
int frees = 0;
void MEGUSTA_DTS_CDECL trackedFree(void* memory) {
    ++frees;
    std::free(memory);
}
}

int main() {
    using namespace megusta::dts;
    bindSharedStorageFree(trackedFree);

    TSMesh_sharedVerts_initialize();
    assert(sharedMeshVerts.count == 0 && sharedMeshVerts.capacity == 0 && !sharedMeshVerts.data);
    sharedMeshVerts.data = std::malloc(32);
    assert(sharedMeshVerts.data);
    TSMesh_sharedVerts_cleanup();
    TSMesh_sharedVerts_cleanup();
    assert(frees == 1 && sharedMeshVerts.data);
    sharedMeshVerts.data = nullptr;

    TSSkinMesh_sharedWeights_initialize();
    sharedSkinWeights.data = std::malloc(24);
    assert(sharedSkinWeights.data);
    TSSkinMesh_sharedWeights_cleanup();
    assert(frees == 2 && sharedSkinWeights.data);
    sharedSkinWeights.data = nullptr;

    TSMesh_decodedNormals_initialize();
    sharedDecodedNormals.data = std::malloc(48);
    assert(sharedDecodedNormals.data);
    TSMesh_decodedNormals_cleanup();
    assert(frees == 3 && sharedDecodedNormals.data);
    sharedDecodedNormals.data = nullptr;

    bindSharedStorageFree(nullptr);
}
