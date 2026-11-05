#include "../src/dts/scratch_release.h"
#include "../src/dts/wrappers/scratch_release_wrappers.h"
#include "../src/dts/shared_storage.h"
#include "../src/dts/shadow_support.h"
#include "../src/dts/scratch_storage.h"
#include <cassert>
#include <cstdlib>

namespace {
void allocate(megusta::dts::DTSVectorStorage& vector,std::uint32_t count,
              std::uint32_t capacity,std::size_t bytes) {
    vector.count=count;vector.capacity=capacity;vector.data=std::malloc(bytes);
    assert(vector.data);
}
void released(const megusta::dts::DTSVectorStorage& vector) {
    assert(vector.count==0 && vector.capacity==0 && vector.data==nullptr);
}
}

int main() {
    using namespace megusta::dts;
    allocate(sharedDecodedNormals,4,16,192);
    allocate(shadowProjectionStorage,3,16,128);
    TSMesh_releaseScratchBuffers_impl();
    released(sharedDecodedNormals);released(shadowProjectionStorage);

    allocate(scratchShapeRead0,5,16,64);
    allocate(scratchShapeRead1,6,16,192);
    DTS_releaseLegacyScratchBuffers_impl();
    released(scratchShapeRead0);released(scratchShapeRead1);

    allocate(scratchShapeRead2,7,16,1024);
    DTS_releaseShapeReadScratchBuffers_impl();
    released(scratchShapeRead2);
}
