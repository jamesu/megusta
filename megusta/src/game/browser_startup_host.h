#pragma once
#ifdef __EMSCRIPTEN__
#include "startup.h"
#include "driver.h"
#include "camera.h"
namespace megusta {
struct BrowserStartupHostContext {
    platform::Runtime& platform;
    DriverAutorun& autorun;
    DriverAutorunServices& autorunServices;
    CameraRuntime& camera;
    int& textureDepth;
    int& textureLimit;
    int& verticalSync;
};
StartupServices makeBrowserStartupServices(BrowserStartupHostContext&);
}
#endif
