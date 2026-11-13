// SPDX-License-Identifier: GPL-2.0

#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace megusta {
inline bool aiDiagnosticsEnabled() {
#ifdef __EMSCRIPTEN__
    static const bool enabled=EM_ASM_INT({return new URLSearchParams(window.location.search).get('aiTrace') === '1';})!=0;
    return enabled;
#else
    return false;
#endif
}
}
