// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "shared_storage.h"

namespace megusta { namespace dts {
extern DTSVectorStorage scratchNodeValues;
extern DTSVectorStorage scratchNodeTranslations;
extern DTSVectorStorage scratchUniformScales;
extern DTSVectorStorage scratchAlignedScales;
extern DTSVectorStorage scratchArbitraryScales;
extern DTSVectorStorage scratchScaleThreads;
extern DTSVectorStorage scratchAuxiliary0;
extern DTSVectorStorage scratchAuxiliary1;
extern DTSVectorStorage scratchShapeRead0;
extern DTSVectorStorage scratchShapeRead1;
extern DTSVectorStorage scratchShapeRead2;
extern DTSVectorStorage scratchShapeRead3;
} }

extern "C" {
#define MEGUSTA_DTS_DECLARE_SCRATCH(name) \
 void name##_initialize(void); \
 void name##_registerCleanup(void); \
 void name##_cleanup(void)
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054eca8);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ecf8);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ecd8);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ed08);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ecc8);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ec98);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ecb8);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ece8);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ee48);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ed28);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054ee68);
MEGUSTA_DTS_DECLARE_SCRATCH(DTS_scratchVector_0054eec0);
void DTS_registeredStartup_004c67b0(void);
void DTS_registeredStartup_004c6820(void);
void DTS_registeredStartup_004c6890(void);
void DTS_registeredStartup_004c6900(void);
void DTS_registeredStartup_004c6970(void);
void DTS_registeredStartup_004c69e0(void);
void DTS_registeredStartup_004c6a50(void);
void DTS_registeredStartup_004c6ac0(void);
void DTS_registeredStartup_004c9310(void);
void DTS_registeredStartup_004c9370(void);
void DTS_registeredStartup_004cc320(void);
void DTS_registeredStartup_004cdba0(void);
void DTS_registeredStartup_004c6790(void);
void DTS_staticInitializer_004c67a0(void);
}
#undef MEGUSTA_DTS_DECLARE_SCRATCH
