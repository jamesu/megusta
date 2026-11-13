// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "types.h"

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_allocShape32_impl(
        megusta::dts::TSShapeAlloc* self,int count);
extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_getPointer32_impl(
        megusta::dts::TSShapeAlloc* self,int count);
extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_allocShape16_impl(
        megusta::dts::TSShapeAlloc* self,int count);
extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_getPointer16_impl(
        megusta::dts::TSShapeAlloc* self,int count);
extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_allocShape8_impl(
        megusta::dts::TSShapeAlloc* self,int count);
extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_getPointer8_impl(
        megusta::dts::TSShapeAlloc* self,int count);
