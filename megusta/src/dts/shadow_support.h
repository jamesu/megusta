// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "shared_storage.h"

namespace megusta { namespace dts {
extern DTSVectorStorage shadowProjectionStorage;
void bindShadowProjectionFree(SharedStorageFree freeFunction);
} }

extern "C" {
void DTS_registeredStartup_004c21c0(void);
void DTS_shadowProjection_initialize(void);
void DTS_shadowProjection_registerCleanup(void);
void DTS_shadowProjection_cleanup(void);
void DTS_billboardMatrix_cleanup_stub(void);
void DTS_shapeRead_cleanup_stub(void);
}
