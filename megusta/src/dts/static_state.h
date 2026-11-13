// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>

namespace megusta { namespace dts {
extern std::uint32_t materialRenderStateWords[8];
extern std::uint32_t billboardBasisStateWords[12];
} }

extern "C" {
void DTS_registeredStartup_004d4700(void);
void DTS_staticInitializer_004d4710(void);
void DTS_registeredStartup_004d5db0(void);
void DTS_staticInitializer_004d5dc0(void);
void DTS_registeredStartup_004d5e20(void);
void DTS_staticInitializer_004d5e30(void);
}
