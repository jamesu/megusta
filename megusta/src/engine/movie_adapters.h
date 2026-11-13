// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>

namespace megusta::engine_movie {
void engineMovie_swapBgr256Square_impl(std::uint8_t* pixels);
bool engineMovie_sendMciCommand_impl(const char* command,void* window,char* result,
    unsigned resultCapacity);
bool engineMovie_isSkipPressed_impl();
}
