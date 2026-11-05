#include "state_transpose_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_CDECL TSShape_rearrangeStates_impl(
        int first,int frames,int tracks,void* data,std::uint32_t stride) {
    megusta::dts::rearrangeStates(first,frames,tracks,data,stride);
}
