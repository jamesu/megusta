// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>

namespace megusta::engine_codec {
int packedCodec_decodeFlaggedBackrefs_impl(const std::uint8_t* input,
    std::uint8_t* output,std::uint32_t outputCapacity);
int packedCodec_decodeLiteralRunBackrefs_impl(const std::uint8_t* input,
    int inputBytes,std::uint8_t* output,int outputBytes);
}
