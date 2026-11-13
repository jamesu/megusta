// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" megusta::dts::TSMaterialList* MEGUSTA_DTS_FASTCALL
TSMaterialList_ctor_impl(megusta::dts::TSMaterialList* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSMaterialList_dtor_impl(
    megusta::dts::TSMaterialList* self);
extern "C" void* MEGUSTA_DTS_THISCALL TSMaterialList_scalarDeletingDtor_impl(
    megusta::dts::TSMaterialList* self,std::uint8_t deleteObject);
extern "C" void MEGUSTA_DTS_FASTCALL TSMaterialList_free_impl(
    megusta::dts::TSMaterialList* self);
extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_remap_impl(
    megusta::dts::TSMaterialList* self,int destination,int source);
extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_allocate_impl(
    megusta::dts::TSMaterialList* self,std::uint32_t count);
extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_load_impl(
    megusta::dts::TSMaterialList* self,std::uint32_t index);
extern "C" void MEGUSTA_DTS_FASTCALL DTS_MaterialList_ctor_impl(
    megusta::dts::DTSMaterialList* self);
extern "C" void MEGUSTA_DTS_FASTCALL DTS_MaterialList_dtor_impl(
    megusta::dts::DTSMaterialList* self);
extern "C" void* MEGUSTA_DTS_THISCALL DTS_MaterialList_scalarDeletingDtor_impl(
    megusta::dts::DTSMaterialList* self,std::uint8_t deleteObject);
extern "C" void MEGUSTA_DTS_THISCALL DTS_MaterialList_load_index_impl(
    megusta::dts::DTSMaterialList* self,std::uint32_t index);
extern "C" int MEGUSTA_DTS_FASTCALL DTS_MaterialList_load_all_impl(
    megusta::dts::DTSMaterialList* self);
extern "C" void MEGUSTA_DTS_THISCALL DTS_MaterialList_unload_impl(
    megusta::dts::DTSMaterialList* self,std::uint32_t index);
extern "C" void MEGUSTA_DTS_FASTCALL DTS_MaterialList_free_impl(
    megusta::dts::DTSMaterialList* self);
extern "C" void MEGUSTA_DTS_FASTCALL TSMaterialList_tsmlTransform_impl(
    megusta::dts::TSMaterialList* self);
extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_push_back_impl(
    megusta::dts::TSMaterialList* self,char* name,std::uint32_t flags,
    int reflectanceMap,int bumpMap,int detailMap,float detailScale,float reflectionAmount);
extern "C" int MEGUSTA_DTS_THISCALL DTS_MaterialList_push_back_impl(
    megusta::dts::DTSMaterialList* self,char* name);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSMaterialList_read_impl(
    megusta::dts::TSMaterialList* self,megusta::ResourceFile* file);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL DTS_MaterialList_read_impl(
    megusta::dts::DTSMaterialList* self,megusta::ResourceFile* file);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL DTS_MaterialList_readText_impl(
    megusta::dts::DTSMaterialList* self,megusta::ResourceFile* file,char first);
