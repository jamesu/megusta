#pragma once

namespace megusta::startup_literals {
// Recovered from the canonical Ghidra database's bytes at 004f22e0 and
// 004f24c4 respectively. The trailing NUL is part of each saved literal.
inline constexpr char consolePrompt[] = ">>>";
inline constexpr char resourcePath[] = ".";
}
