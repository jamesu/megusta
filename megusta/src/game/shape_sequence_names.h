// SPDX-License-Identifier: GPL-2.0

#pragma once
namespace megusta {
// Original pointer table 00502b98, read from the canonical Ghidra database by
// InspectStartupData.java; order defines the DSQ import mask bits.
inline constexpr const char* gameShapeSequenceNames[]={
    "putt","par","bogy","wait","pre","pre2","bird","miss","par2","shl",nullptr
};
// Fixed ten-slot table at 00502cb4; null entries intentionally skip voices.
inline constexpr const char* gamePlayerVoiceNames[]={nullptr,"PAR","BOGY","WAIT",nullptr,nullptr,"BIRD","MISS","PAR2",nullptr};
}
