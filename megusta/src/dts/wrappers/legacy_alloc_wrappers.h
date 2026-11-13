// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_getDWordCount32_impl(void);
extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_getDWordCount16_impl(void);
extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_getDWordCount8_impl(void);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldInitAlloc32_impl(void);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldInitAlloc16_impl(void);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldInitAlloc8_impl(void);
extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_oldAllocOffset16_impl(void);
extern "C" std::uint32_t* MEGUSTA_DTS_CDECL DTSLegacy_oldAlloc32_impl(
        std::uint32_t** allocation,int count);
extern "C" std::uint16_t* MEGUSTA_DTS_CDECL DTSLegacy_oldAlloc16_impl(
        std::uint16_t** allocation,int count);
extern "C" std::uint8_t* MEGUSTA_DTS_CDECL DTSLegacy_oldAlloc8_impl(
        std::uint8_t** allocation,int count);
extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_readAlloc32_append_impl(
        megusta::ResourceFile* file,std::uint32_t** allocation);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc32_store_impl(
        megusta::ResourceFile* file,std::uint32_t* value);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc_32_impl(
        megusta::ResourceFile* file,std::uint32_t** allocation,std::uint32_t count);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc_16_impl(
        megusta::ResourceFile* file,std::uint16_t** allocation,std::uint32_t count);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc_8_impl(
        megusta::ResourceFile* file,std::uint8_t** allocation,std::uint32_t count);
extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldAllocGuard_impl(
        std::uint32_t** allocation32,std::uint16_t** allocation16,std::uint8_t** allocation8);
