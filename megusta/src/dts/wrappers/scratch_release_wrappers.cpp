#include "scratch_release_wrappers.h"
#include "../scratch_release.h"
using namespace megusta::dts;

extern "C" void TSMesh_releaseScratchBuffers_impl(void) {
    releaseMeshScratchBuffers();
}

extern "C" void DTS_releaseLegacyScratchBuffers_impl(void) {
    releaseLegacyScratchBuffers();
}

extern "C" void DTS_releaseShapeReadScratchBuffers_impl(void) {
    releaseShapeReadScratchBuffers();
}
