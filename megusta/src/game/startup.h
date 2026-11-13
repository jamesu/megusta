// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "driver.h"
#include "game_config.h"
#include "engine/resource_runtime.h"
#include "platform/runtime.h"
namespace megusta {
// Original globals normalized into one startup state. Defaults for data words
// absent from the saved exports must be supplied by the caller.
struct StartupPreferences {
    // Defaults read from the original data globals by gameRuntime_main_impl:
    // 00524618, 0052461c, 00524620, 00524628, and 0054f2f8.
    int screenDepth=32;
    int zDepth=16;
    int soundDepth=8;
    int textureDepth=8;
    int textureLimitIndex=0;
    int vsyncMode=-1;
};
struct StartupState {
    bool intro=true,buildTables=false,demo=false,forcePix=false;
    int sceneLoadMethod=0,alpha=0;
    StartupPreferences preferences;
};
struct StartupServices {
    void* context;
    // These are unresolved native boundaries, not optional success stubs.
    void (*shellConfig)(void*,const char*);
    void (*autorun)(void*,int,int,bool);
    void (*cameraDelay)(void*,int);
    void (*applyPreferences)(void*,const StartupPreferences&);
    void (*movieCleanup)(void*,const char*,int);
    int (*playMovie)(void*,const char*,int,int,int,int,int);
};
void parseStartupArguments(int argc,char* const* argv,StartupState&,const StartupServices&);
int runGameStartup(int argc,char** argv,ResourceRuntime&,GameConfig&,
    DriverViewport&,platform::Runtime&,StartupState&,DriverInitialization&,const StartupServices&);
struct GameEntryServices {
    ResourceRuntime& resources;
    GameConfig& config;
    DriverViewport& viewport;
    platform::Runtime& platform;
    StartupState& startup;
    DriverInitialization& driver;
    const StartupServices& host;
    int argc;
    char** argv;
    void* nativeInstance=nullptr;
};
void bindGameEntryServices(GameEntryServices&);
void gameRuntimeStartupBridge(int,char**);
void engineRuntimeWinMainBootstrap(void*);
}
