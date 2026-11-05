#pragma once

namespace megusta {

// Shared game-space to PLIB-space basis referenced by the executable at
// 00506ab8. The exact 16 little-endian float values were read from the
// canonical Ghidra program listing; camera and GUI preview paths use the same
// matrix in the original.
inline constexpr float gameCoordinateConversionMatrix[16] = {
    1.0f, 0.0f,  0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 1.0f,  0.0f, 0.0f,
    0.0f, 0.0f,  0.0f, 1.0f,
};

} // namespace megusta
