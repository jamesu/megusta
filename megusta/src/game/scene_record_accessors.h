// SPDX-License-Identifier: GPL-2.0

#pragma once

namespace megusta {

// The executable's 004189d0 returns the address sixteen bytes into the
// supplied record. The caller treats that address as a three-float point.
void* SceneRecord_getPointAt10_impl(void* record);

} // namespace megusta
