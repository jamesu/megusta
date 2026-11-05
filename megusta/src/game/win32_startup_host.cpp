#include "win32_startup_host.h"
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
namespace megusta {
namespace {
Win32StartupHostContext& host(void* value) {
    return *static_cast<Win32StartupHostContext*>(value);
}
void launchConfiguration(void*,const char* executable) {
    // The saved game invokes MinigolfConfig.exe when MINIGOLF.CFG is absent.
    (void)ShellExecuteA(nullptr,"open",executable,nullptr,nullptr,SW_SHOWNORMAL);
}
void configureAutorun(void* value,int menuSeconds,int gameSeconds,bool startNow) {
    auto& s=host(value);
    s.autorun.configure(menuSeconds,gameSeconds,startNow,s.autorunServices);
}
void setCameraDelay(void* value,int delay) {
    Camera::setControlPathStep(host(value).camera,delay);
}
void applyPreferences(void* value,const StartupPreferences& preferences) {
    auto& s=host(value);
    s.platform.state.colorBits=preferences.screenDepth;
    s.platform.state.depthBits=preferences.zDepth;
    s.textureDepth=preferences.textureDepth;
    s.textureLimit=preferences.textureLimitIndex;
    s.verticalSync=preferences.vsyncMode;
}
}
StartupServices makeWin32StartupServices(Win32StartupHostContext& context) {
    StartupServices services{};
    services.context=&context;
    services.shellConfig=launchConfiguration;
    services.autorun=configureAutorun;
    services.cameraDelay=setCameraDelay;
    services.applyPreferences=applyPreferences;
    // Movies use the existing Win32 movie adapter; cleanup is registered by
    // runGameStartup when no host override is supplied.
    services.movieCleanup=nullptr;
    services.playMovie=nullptr;
    return services;
}
}
#endif
