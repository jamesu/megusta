// SPDX-License-Identifier: GPL-2.0

// Generated from canonical Ghidra exports.
#pragma once
#include <cstdint>
namespace megusta { namespace task_constants {
inline constexpr int modes[] = {1,2,3,5,6,10,15,30};
inline constexpr std::uint32_t masks[] = {0x00000001u,0x00010001u,0x40100401u,0x10410410u,0x42108421u,0x92492492u,0xaaaaaaaau,0xffffffffu};
// Original 004f30e0
inline constexpr char warningFormat[] = "TASK: WARNING: task has bad fps=%d\n";
// Original 004f9c94
inline constexpr char emptyTupleFormat[] = "()";
// Original 005033b0
inline constexpr char fatalFormat[] = "FATAL: %s @ %d: %s\n";
} }
