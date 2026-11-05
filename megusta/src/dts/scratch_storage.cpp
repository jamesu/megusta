#include "scratch_storage.h"
#include <cstdlib>

namespace megusta { namespace dts {
DTSVectorStorage scratchNodeValues;
DTSVectorStorage scratchNodeTranslations;
DTSVectorStorage scratchUniformScales;
DTSVectorStorage scratchAlignedScales;
DTSVectorStorage scratchArbitraryScales;
DTSVectorStorage scratchScaleThreads;
DTSVectorStorage scratchAuxiliary0;
DTSVectorStorage scratchAuxiliary1;
DTSVectorStorage scratchShapeRead0;
DTSVectorStorage scratchShapeRead1;
DTSVectorStorage scratchShapeRead2;
DTSVectorStorage scratchShapeRead3;
namespace {
std::uint8_t cleanupBits = 0;
void initializeVector(DTSVectorStorage& value) {
    value.count = 0;
    value.data = nullptr;
    value.capacity = 0;
}
void freeVector(DTSVectorStorage& value) {
    if (value.data) freeSharedStorageAllocation(value.data);
}
void freeVectorOnce(DTSVectorStorage& value, std::uint8_t bit) {
    if ((cleanupBits & bit) == 0) {
        cleanupBits = static_cast<std::uint8_t>(cleanupBits | bit);
        freeVector(value);
    }
}
} } }

using namespace megusta::dts;

#define DEFINE_GUARDED_SCRATCH(name, value, bit) \
extern "C" void name##_initialize(void) { initializeVector(value); } \
extern "C" void name##_registerCleanup(void) { std::atexit(name##_cleanup); } \
extern "C" void name##_cleanup(void) { freeVectorOnce(value, bit); }

#define DEFINE_UNGUARDED_SCRATCH(name, value) \
extern "C" void name##_initialize(void) { initializeVector(value); } \
extern "C" void name##_registerCleanup(void) { std::atexit(name##_cleanup); } \
extern "C" void name##_cleanup(void) { freeVector(value); }

DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054eca8, scratchNodeValues, 0x01)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ecf8, scratchNodeTranslations, 0x02)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ecd8, scratchUniformScales, 0x04)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ed08, scratchAlignedScales, 0x08)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ecc8, scratchArbitraryScales, 0x10)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ec98, scratchScaleThreads, 0x20)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ecb8, scratchAuxiliary0, 0x40)
DEFINE_GUARDED_SCRATCH(DTS_scratchVector_0054ece8, scratchAuxiliary1, 0x80)
DEFINE_UNGUARDED_SCRATCH(DTS_scratchVector_0054ee48, scratchShapeRead0)
DEFINE_UNGUARDED_SCRATCH(DTS_scratchVector_0054ed28, scratchShapeRead1)
DEFINE_UNGUARDED_SCRATCH(DTS_scratchVector_0054ee68, scratchShapeRead2)
DEFINE_UNGUARDED_SCRATCH(DTS_scratchVector_0054eec0, scratchShapeRead3)

#define DEFINE_STARTUP(address, name) \
extern "C" void DTS_registeredStartup_##address(void) { \
    name##_initialize(); name##_registerCleanup(); \
}
DEFINE_STARTUP(004c67b0, DTS_scratchVector_0054eca8)
DEFINE_STARTUP(004c6820, DTS_scratchVector_0054ecf8)
DEFINE_STARTUP(004c6890, DTS_scratchVector_0054ecd8)
DEFINE_STARTUP(004c6900, DTS_scratchVector_0054ed08)
DEFINE_STARTUP(004c6970, DTS_scratchVector_0054ecc8)
DEFINE_STARTUP(004c69e0, DTS_scratchVector_0054ec98)
DEFINE_STARTUP(004c6a50, DTS_scratchVector_0054ecb8)
DEFINE_STARTUP(004c6ac0, DTS_scratchVector_0054ece8)
DEFINE_STARTUP(004c9310, DTS_scratchVector_0054ee48)
DEFINE_STARTUP(004c9370, DTS_scratchVector_0054ed28)
DEFINE_STARTUP(004cc320, DTS_scratchVector_0054ee68)
DEFINE_STARTUP(004cdba0, DTS_scratchVector_0054eec0)
extern "C" void DTS_registeredStartup_004c6790(void) {}
extern "C" void DTS_staticInitializer_004c67a0(void) {}

#undef DEFINE_STARTUP
#undef DEFINE_UNGUARDED_SCRATCH
#undef DEFINE_GUARDED_SCRATCH
