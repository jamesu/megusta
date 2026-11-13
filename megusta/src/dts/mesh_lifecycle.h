// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "mesh.h"
#include "vector_storage.h"

namespace megusta { namespace dts {
using DisplayListDelete = void (*)(std::uint32_t list,int range);
void bindDisplayListDelete(DisplayListDelete callback);
void destroyTSMesh(TSMesh* self);
void constructTSSkinMesh(TSSkinMesh* self);
void destroyTSSkinMesh(TSSkinMesh* self);
void destroyTSDecalMesh(TSDecalMesh* self);
void deleteDisplayListIfBound(std::uint32_t list);
} }

extern "C" {
void DTS_scratchMesh_cleanup(void);
void DTS_scratchDecal_cleanup(void);
void DTS_scratchSkin_cleanup(void);
void DTS_scratchSorted_cleanup(void);
}
