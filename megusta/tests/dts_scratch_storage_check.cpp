#include "../src/dts/scratch_storage.h"
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
    using namespace megusta::dts;
    bindSharedStorageFree(trackedFree);

    DTS_scratchVector_0054eca8_initialize();
    assert(scratchNodeValues.count == 0 && scratchNodeValues.capacity == 0);
    scratchNodeValues.data = std::malloc(32);
    assert(scratchNodeValues.data);
    DTS_scratchVector_0054eca8_cleanup();
    DTS_scratchVector_0054eca8_cleanup();
    assert(freeCount == 1 && scratchNodeValues.data);
    scratchNodeValues.data = nullptr;

    DTS_scratchVector_0054ee48_initialize();
    scratchShapeRead0.data = std::malloc(48);
    assert(scratchShapeRead0.data);
    DTS_scratchVector_0054ee48_cleanup();
    assert(freeCount == 2 && scratchShapeRead0.data);
    scratchShapeRead0.data = nullptr;

    bindSharedStorageFree(nullptr);
}
