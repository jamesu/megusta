#pragma once
#include <cstddef>
#include <cstdint>
#include "vector_storage.h"
namespace megusta { namespace dts {
// Original parameter order and caller-owned buffer interface at 004ca170.
void rearrangeStates(int first,int frames,int tracks,void* data,unsigned stride);
// 004ca170: convert frame-major records to track-major records in place.
// bytes is the accessible extent of data, including records before first.
bool rearrangeStates(void* data,std::size_t bytes,int first,int frames,int tracks,unsigned stride);
} }

