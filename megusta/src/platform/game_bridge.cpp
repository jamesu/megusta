#include "game_bridge.h"
#include "game/driver.h"
#include "game/camera.h"
#include <stdexcept>
namespace megusta::platform {
namespace { Runtime* active=nullptr;
Runtime& platform() { if (!active) throw std::logic_error("Game platform adapter is not bound"); return *active; }
}
void bindGamePlatform(Runtime* runtime) { active=runtime; }
std::uint32_t gameMilliseconds() { return platform().milliseconds(); }
void gameSleepMicroseconds(std::uint32_t usec) { platform().sleepMicroseconds(usec); }
int engineRuntime_sleepMicroseconds_adapter(std::uint32_t usec) {
    gameSleepMicroseconds(usec);
    return 0;
}
int gameQuery(int code) { return static_cast<int>(platform().query(code)); }
int gameSystemProperty(int code) { return platform().systemProperty(code); }
std::uint32_t gameModifiers() { return static_cast<std::uint32_t>(platform().activeModifiers()); }
void gameSetCursorMode(int mode) { platform().setCursorMode(static_cast<CursorMode>(mode)); }
void gameWarpCursor(int x,int y) { platform().warpCursor(x,y); }
void gameEnableJoystick(bool enabled) { platform().registerJoystick(enabled); }
std::uint32_t gameMillisecondsForContext(void*) { return gameMilliseconds(); }
std::uint32_t gameModifiersForContext(void*) { return gameModifiers(); }
void gameSleepForContext(void*,std::uint32_t usec) { gameSleepMicroseconds(usec); }
void bindCameraPlatformServices(CameraServices& services) { services.setMouseCapture=gameSetCursorMode; }
void bindDriverCallbacks(DriverPlatformBinding& binding) {
    bindGamePlatform(&binding.runtime);
    binding.services.timeMilliseconds=gameMillisecondsForContext;
    binding.services.sleepMicroseconds=gameSleepForContext;
    binding.input.joystickDeviceId=[](void*){return gameSystemProperty(3);};
    auto& cb=binding.runtime.callbacks; cb.context=&binding;
    cb.frame=[](void* p){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.frame(b.services);};
    cb.idle=[](void* p){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.idleUpdate(b.services);};
    cb.keyDown=[](void* p,std::uint8_t key){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.keyDownCallback(key,b.input);};
    cb.specialDown=[](void* p,int key){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.specialKeyDownCallback(key,b.input);};
    cb.specialUp=[](void* p,int key){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.specialKeyUpCallback(key,b.input);};
    cb.mouseButton=[](void* p,int button,int state,int x,int y){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.mouseButtonCallback(button,state,x,y,b.input);};
    cb.mouseMotion=[](void* p,int x,int y){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.mouseMotionCallback(x,y,b.input);};
    cb.passiveMotion=cb.mouseMotion;
    cb.joystick=[](void* p,std::uint32_t buttons,int count,const float* axes){auto& b=*static_cast<DriverPlatformBinding*>(p); b.driver.joystickCallback(buttons,count,axes,b.input);};
}
}
