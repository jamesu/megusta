// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <cstddef>
#include <cstdint>

namespace megusta {

// Shared 12-byte result used by engineFile_queryStat and its resource-mount
// callback. The executable checks kindFlags at offset 0, consumes byteCount
// at offset 4, and load_source_module passes the value at offset 8 to the
// compiled-module timestamp check.
struct EngineFileStatus {
    std::uint16_t kindFlags=0;
    std::uint16_t reserved=0;
    std::int32_t byteCount=0;
    std::int32_t modificationTime=0;
};

static_assert(sizeof(EngineFileStatus)==12);
static_assert(offsetof(EngineFileStatus,kindFlags)==0);
static_assert(offsetof(EngineFileStatus,byteCount)==4);
static_assert(offsetof(EngineFileStatus,modificationTime)==8);

} // namespace megusta
