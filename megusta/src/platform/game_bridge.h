#pragma once
#include "runtime.h"
namespace megusta { class Driver; struct DriverInputServices; struct DriverServices; struct CameraServices; }
namespace megusta::platform {
// Retain this record as long as callbacks may run; references are borrowed.
struct DriverPlatformBinding { Runtime& runtime; Driver& driver; DriverInputServices& input; DriverServices& services; };
void bindDriverCallbacks(DriverPlatformBinding&);
void bindGamePlatform(Runtime*);
std::uint32_t gameMilliseconds();
void gameSleepMicroseconds(std::uint32_t);
int engineRuntime_sleepMicroseconds_adapter(std::uint32_t);
int gameQuery(int);
int gameSystemProperty(int);
std::uint32_t gameModifiers();
void gameSetCursorMode(int);
void gameWarpCursor(int,int);
void gameEnableJoystick(bool);
std::uint32_t gameMillisecondsForContext(void*);
std::uint32_t gameModifiersForContext(void*);
void gameSleepForContext(void*,std::uint32_t);
void bindCameraPlatformServices(CameraServices&);
}
