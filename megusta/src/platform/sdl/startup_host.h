// SPDX-License-Identifier: GPL-2.0

#pragma once
#ifdef MEGUSTA_SDL
#include "game/startup.h"
#include "game/driver.h"
#include "game/camera.h"
namespace megusta {
struct SdlStartupHostContext {
    platform::Runtime& platform;
    DriverAutorun& autorun;
    DriverAutorunServices& autorunServices;
    CameraRuntime& camera;
    int& textureDepth;
    int& textureLimit;
    int& verticalSync;
};
StartupServices makeSdlStartupServices(SdlStartupHostContext&);
}
#endif
