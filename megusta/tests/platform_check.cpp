#include "platform/runtime.h"
#ifdef _WIN32
#include "platform/win32.h"
#endif
#include <cmath>
#include <cstdio>
int checkPlatformRuntime() {
    using namespace megusta::platform;
    HeadlessRuntime arguments;
    if (arguments.state.fullscreenMode!=FullscreenMode::Borderless) return 21;
    char input[][40]={"C:\\games\\golf.test.exe","-FuLl","-window","-bitdepth:24","-bitdepth:17","-MuLtI"};
    char* argv[6]; for (int i=0;i<6;++i) argv[i]=input[i];
    arguments.initializeFromArgs(6,argv);
    if (arguments.state.fullscreenRequested || !arguments.state.fullscreenForbidden ||
        arguments.state.colorBits!=24 || !arguments.state.allowMultipleInstances ||
        arguments.state.className!="golf.test") return 17;
    arguments.requestInitialFullscreen();
    if (arguments.state.fullscreenRequested) return 18;
    char full[]="-full",depth32[]="-bitdepth:32",next[]="other.exe";
    char* again[]={next,full,depth32};
    arguments.initializeFromArgs(3,again);
    if (!arguments.state.fullscreenRequested || arguments.state.colorBits!=32 ||
        arguments.state.className!="golf.test") return 19;
    HeadlessRuntime posix;
    char path[]="/games/minigolf",badDepth[]="-bitdepth:40";
    char* posixArgs[]={path,badDepth};
    posix.initializeFromArgs(2,posixArgs);
    if (posix.state.className!="minigolf" || posix.state.colorBits!=32) return 20;
    char exclusive[]="-fullscreen:exclusive",borderless[]="-fullscreen:borderless";
    char* policies[]={path,exclusive};
    posix.initializeFromArgs(2,policies);
    if (posix.state.fullscreenMode!=FullscreenMode::Exclusive || posix.state.fullscreenRequested) return 22;
    policies[1]=borderless;
    posix.initializeFromArgs(2,policies);
    if (posix.state.fullscreenMode!=FullscreenMode::Borderless || posix.state.fullscreenRequested) return 23;
    HeadlessRuntime runtime;
    runtime.setInitialSize(320,200); runtime.requestInitialFullscreen();
    if (!runtime.state.fullscreenRequested || !runtime.create("headless")) return 1;
    runtime.setInitialSize(999,999);
    if (runtime.query(1)!=320 || runtime.query(2)!=200 || runtime.query(99)!=0) return 2;
    runtime.sleepMicroseconds(1999);
    if (runtime.query(0)!=1) return 3;
    runtime.clock=0xffffffffu; runtime.sleepMicroseconds(1000);
    if (runtime.milliseconds()!=0) return 4;
    runtime.state.modifiers=-1;
    if (runtime.activeModifiers()!=0) return 5;
    runtime.state.modifiers=Shift|Alt;
    if (runtime.activeModifiers()!=5) return 6;
    struct Probe { int frames=0,idle=0,joy=0; unsigned buttons=0; float axis=0; } probe;
    runtime.callbacks.context=&probe;
    runtime.callbacks.frame=[](void* p){++static_cast<Probe*>(p)->frames;};
    runtime.callbacks.idle=[](void* p){++static_cast<Probe*>(p)->idle;};
    runtime.callbacks.joystick=[](void* p,unsigned buttons,int count,const float* axes){auto& q=*static_cast<Probe*>(p);q.joy+=count==4;q.buttons=buttons;q.axis=axes[0];};
    runtime.oneFrame(); runtime.state.minimized=true; runtime.oneFrame();
    if (probe.frames!=1 || probe.idle!=2 || runtime.presented!=1 || runtime.clock!=10) return 7;
    runtime.state.joystickAvailable=true;
    runtime.joystick.axes[0]=-500; runtime.joystick.axes[1]=1000; runtime.joystick.axes[2]=316; runtime.joystick.axes[3]=317;
    runtime.joystick.buttons[4]=0x80; runtime.joystick.buttons[15]=0x80;
    runtime.registerJoystick(true,5); runtime.dispatchJoystick();
    if (probe.joy!=1 || probe.buttons!=0x8001u || probe.axis!=-0.25f || runtime.state.joystickButtons!=0 || runtime.timerInterval!=5) return 8;
    if (runtime.state.joystickAxes[1]!=1 || runtime.state.joystickAxes[2]!=0 || runtime.state.joystickAxes[3]<=0.1f) return 9;
    runtime.registerJoystick(false); runtime.dispatchJoystick(); if (probe.joy!=1) return 10;
    runtime.setCursorMode(CursorMode::Captured); runtime.warpCursor(15,20);
    if (!runtime.state.cursorCaptured || runtime.cursorX!=15 || runtime.cursorY!=20) return 11;
    runtime.setCursorMode(CursorMode::Released); if (runtime.state.cursorCaptured) return 12;
    runtime.shutdown(); if (runtime.pumpEvents() || runtime.activeModifiers()!=0) return 13;
    auto caps=runtime.capabilities(); if (caps.graphics || caps.processPriority || caps.blockingSleep) return 14;
#ifdef _WIN32
    Win32Runtime native;
    native.setInitialSize(320,200);
    if (native.systemProperty(3)!=1 || native.systemProperty(99)!=0 || native.query(1)!=320 || native.activeModifiers()!=0) return 15;
    const auto start=native.milliseconds(); native.sleepMicroseconds(0);
    if (static_cast<std::uint32_t>(native.milliseconds()-start)>10000u) return 16;
#endif
    std::puts("platform: headless clock, frame/idle order, joystick shaping/button mapping and cursor state passed");
    std::puts("platform: startup option precedence, depth validation and class-name extraction passed");
#ifdef _WIN32
    std::puts("platform: Win32 timing and system-query checks passed (no window or hardware polling exercised)");
#endif
    return 0;
}
