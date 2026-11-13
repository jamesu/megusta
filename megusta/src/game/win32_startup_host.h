// SPDX-License-Identifier: GPL-2.0

#pragma once
#ifdef _WIN32
#include "startup.h"
#include "driver.h"
#include "camera.h"
namespace megusta {
// Concrete host side of 00402360. The referenced objects outlive the
// StartupServices callbacks returned here.
struct Win32StartupHostContext {
    platform::Runtime& platform;
    DriverAutorun& autorun;
    DriverAutorunServices& autorunServices;
    CameraRuntime& camera;
    int& textureDepth;
    int& textureLimit;
    int& verticalSync;
};
StartupServices makeWin32StartupServices(Win32StartupHostContext&);
}
#endif
