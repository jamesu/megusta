#include "../src/dts/shadow_support.h"
#include <cassert>
#include <cstdlib>

namespace {
int freeCount = 0;
void MEGUSTA_DTS_CDECL trackedFree(void* memory) {
    ++freeCount;
    std::free(memory);
}
}

int main() {
    using megusta::dts::shadowProjectionStorage;
    megusta::dts::bindShadowProjectionFree(trackedFree);
    DTS_shadowProjection_initialize();
    assert(shadowProjectionStorage.count == 0);
    assert(shadowProjectionStorage.capacity == 0);
    assert(shadowProjectionStorage.data == nullptr);
    shadowProjectionStorage.data = std::malloc(64);
    assert(shadowProjectionStorage.data);
    DTS_shadowProjection_cleanup();
    assert(freeCount == 1 && shadowProjectionStorage.data);
    shadowProjectionStorage.data = nullptr;
    DTS_billboardMatrix_cleanup_stub();
    DTS_shapeRead_cleanup_stub();
}
