#include "shadow_support.h"
#include <cstdlib>

namespace megusta { namespace dts {
DTSVectorStorage shadowProjectionStorage;
namespace {
SharedStorageFree shadowFree = std::free;
}
void bindShadowProjectionFree(SharedStorageFree freeFunction) {
    shadowFree = freeFunction ? freeFunction : std::free;
}
} }

extern "C" void DTS_shadowProjection_initialize(void) {
    auto& storage = megusta::dts::shadowProjectionStorage;
    storage.count = 0;
    storage.data = nullptr;
    storage.capacity = 0;
}
extern "C" void DTS_shadowProjection_registerCleanup(void) {
    std::atexit(DTS_shadowProjection_cleanup);
}
extern "C" void DTS_shadowProjection_cleanup(void) {
    auto& storage = megusta::dts::shadowProjectionStorage;
    if (storage.data) megusta::dts::shadowFree(storage.data);
}
extern "C" void DTS_registeredStartup_004c21c0(void) {
    DTS_shadowProjection_initialize();
    DTS_shadowProjection_registerCleanup();
}
extern "C" void DTS_billboardMatrix_cleanup_stub(void) {}
extern "C" void DTS_shapeRead_cleanup_stub(void) {}
