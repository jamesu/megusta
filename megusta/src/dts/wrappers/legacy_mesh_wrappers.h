#pragma once
#include "types.h"

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAllocMesh_impl(
        megusta::ResourceFile* file,std::uint32_t** words32,
        std::uint16_t** words16,std::uint8_t** bytes8,int meshType);
