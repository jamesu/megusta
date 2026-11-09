#include "browser_startup_host.h"
#ifdef __EMSCRIPTEN__
namespace megusta {
namespace {
BrowserStartupHostContext& host(void* value) { return *static_cast<BrowserStartupHostContext*>(value); }
void launchConfiguration(void*,const char*) {}
void configureAutorun(void* value,int menuSeconds,int gameSeconds,bool startNow) {
    auto& s=host(value);
    s.autorun.configure(menuSeconds,gameSeconds,startNow,s.autorunServices);
}
void setCameraDelay(void* value,int delay) { Camera::setControlPathStep(host(value).camera,delay); }
void applyPreferences(void* value,const StartupPreferences& preferences) {
    auto& s=host(value);
    s.platform.state.colorBits=preferences.screenDepth;
    s.platform.state.depthBits=preferences.zDepth;
    s.textureDepth=preferences.textureDepth;
    s.textureLimit=preferences.textureLimitIndex;
    s.verticalSync=preferences.vsyncMode;
}
int playMovie(void*,const char*,int,int,int,int,int) { return 1; }
}
StartupServices makeBrowserStartupServices(BrowserStartupHostContext& context) {
    StartupServices services{};
    services.context=&context;
    services.shellConfig=launchConfiguration;
    services.autorun=configureAutorun;
    services.cameraDelay=setCameraDelay;
    services.applyPreferences=applyPreferences;
    services.playMovie=playMovie;
    services.movieCleanup=[](void*,const char*,int){};
    return services;
}
}
#endif
