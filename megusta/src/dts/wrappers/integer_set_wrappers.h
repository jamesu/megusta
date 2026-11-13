// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_clearAll_impl(
        megusta::dts::TSIntegerSet* self,std::uint32_t count);
extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_setAll_impl(
        megusta::dts::TSIntegerSet* self,std::uint32_t count);
extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSIntegerSet_testAll_impl(
        const megusta::dts::TSIntegerSet* self,std::uint32_t count);
extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_overlap_impl(
        megusta::dts::TSIntegerSet* self,const megusta::dts::TSIntegerSet* other);
extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_takeAway_impl(
        megusta::dts::TSIntegerSet* self,const megusta::dts::TSIntegerSet* other);
extern "C" int MEGUSTA_DTS_FASTCALL TSIntegerSet_start_impl(
        const megusta::dts::TSIntegerSet* self);
extern "C" int MEGUSTA_DTS_FASTCALL TSIntegerSet_end_impl(
        const megusta::dts::TSIntegerSet* self);
extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_next_impl(
        const megusta::dts::TSIntegerSet* self,std::uint32_t* index);
extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_copy_impl(
        megusta::dts::TSIntegerSet* self,const void* other);
extern "C" megusta::dts::TSIntegerSet* MEGUSTA_DTS_FASTCALL TSIntegerSet_ctor_impl(
        megusta::dts::TSIntegerSet* self);
extern "C" megusta::dts::TSIntegerSet* MEGUSTA_DTS_THISCALL TSIntegerSet_copyCtor_impl(
        megusta::dts::TSIntegerSet* self,const megusta::dts::TSIntegerSet* other);
extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_read_impl(
        megusta::dts::TSIntegerSet* self,megusta::ResourceFile* file);
