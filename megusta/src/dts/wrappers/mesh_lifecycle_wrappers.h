// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_FASTCALL TSMesh_dtor_impl(megusta::dts::TSMesh* self);
extern "C" void* MEGUSTA_DTS_THISCALL TSMesh_scalarDeletingDtor_impl(
        megusta::dts::TSMesh* self,std::uint8_t deleteObject);
extern "C" void MEGUSTA_DTS_FASTCALL TSSkinMesh_ctor_impl(megusta::dts::TSSkinMesh* self);
extern "C" void* MEGUSTA_DTS_THISCALL TSSkinMesh_scalarDeletingDtor_impl(
        megusta::dts::TSSkinMesh* self,std::uint8_t deleteObject);
extern "C" void MEGUSTA_DTS_FASTCALL TSDecalMesh_dtor_impl(megusta::dts::TSDecalMesh* self);
extern "C" void* MEGUSTA_DTS_THISCALL TSDecalMesh_scalarDeletingDtor_impl(
        megusta::dts::TSDecalMesh* self,std::uint8_t deleteObject);
