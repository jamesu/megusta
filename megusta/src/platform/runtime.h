// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include <string>
namespace megusta::platform {
enum class Query : int { Time=0,Width=1,Height=2 };
enum Modifier : unsigned { Shift=1,Control=2,Alt=4 };
enum class CursorMode : int { Released=15,Captured=16 };
// Compatibility policy; Borderless keeps the desktop display mode unchanged.
enum class FullscreenMode { Borderless,Exclusive };
struct JoystickSample { std::int32_t axes[4]{}; std::uint8_t buttons[32]{}; };
struct Callbacks {
    void* context=nullptr;
    void (*frame)(void*)=nullptr;
    void (*idle)(void*)=nullptr;
    void (*keyDown)(void*,std::uint8_t)=nullptr;
    void (*specialDown)(void*,int)=nullptr;
    void (*specialUp)(void*,int)=nullptr;
    void (*mouseButton)(void*,int,int,int,int)=nullptr;
    void (*mouseMotion)(void*,int,int)=nullptr;
    void (*passiveMotion)(void*,int,int)=nullptr;
    void (*joystick)(void*,std::uint32_t,int,const float*)=nullptr;
    void (*error)(void*,const char*)=nullptr;
    void (*pointerReset)(void*)=nullptr;
};
struct WindowState {
    int width=0,height=0;
    bool created=false,active=false,minimized=false;
    bool fullscreenRequested=false,fullscreenForbidden=false;
    FullscreenMode fullscreenMode=FullscreenMode::Borderless;
    bool allowMultipleInstances=false,argumentsInitialized=false;
    int desktopWidth=0,desktopHeight=0,desktopBits=0;
    int colorBits=32,depthBits=24; // Compatibility defaults; startup supplies recovered preferences.
    std::string className="MegustaPlatformWindow";
    bool cursorCaptured=false,joystickAvailable=false;
    bool legacyWindowFlag=false; // 0054efcc: setter recovered, consumer semantics unavailable.
    bool exitOnEscape=false; // Explicit compatibility policy, not the recovered flag above.
    int modifiers=0,priority=1;
    std::uint32_t joystickButtons=0;
    float joystickAxes[4]{};
};
struct Capabilities { bool nativeWindow,graphics,cursorWarp,processPriority,blockingSleep; };
class Runtime {
public:
    virtual ~Runtime()=default;
    virtual void drawableSize(int& width,int& height)const { width=state.width;height=state.height; }
    WindowState state;
    Callbacks callbacks;
    void setInitialSize(int,int);
    void requestInitialFullscreen();
    virtual void initializeFromArgs(int argc,char* const* argv);
    std::uint32_t query(int) const;
    int activeModifiers() const;
    void setLegacyWindowFlag(int value) { state.legacyWindowFlag=value!=0; }
    void registerJoystick(bool enabled,unsigned intervalMs=0);
    void dispatchJoystick();
    void oneFrame();
    virtual void runMainLoop();
    // The original raw DI axis/button convention is normalized here once.
    void applyJoystickSample(const JoystickSample&);
    virtual bool create(const char* title)=0;
    virtual Capabilities capabilities() const=0;
    virtual void* nativeWindowHandle() const { return nullptr; } // Headless/browser have no HWND.
    virtual bool pumpEvents()=0;
    virtual void finishAndPresent()=0;
    virtual void shutdown()=0;
    virtual std::uint32_t milliseconds() const=0;
    virtual void sleepMicroseconds(std::uint32_t)=0;
    virtual int systemProperty(int) const=0;
    virtual void updateModifiers()=0;
    virtual void setPriority(int)=0;
    virtual void setCursorMode(CursorMode)=0;
    virtual void warpCursor(int,int)=0;
    virtual bool pollJoystick(JoystickSample&)=0;
    virtual void setJoystickTimer(unsigned)=0;
protected:
    bool joystickRegistered=false;
    bool joystickRequested=false;
};
// Deterministic headless backend: sleep advances its clock without blocking.
// This same implementation is usable by Wasm/Node, with no DOM/Windows types.
class HeadlessRuntime final:public Runtime {
public:
    bool running=false;
    std::uint32_t clock=0,presented=0;
    int cursorX=0,cursorY=0;
    unsigned timerInterval=0;
    JoystickSample joystick;
    bool create(const char*) override;
    Capabilities capabilities() const override { return {false,false,true,false,false}; }
    bool pumpEvents() override { return running; }
    void finishAndPresent() override { ++presented; }
    void shutdown() override;
    std::uint32_t milliseconds() const override { return clock; }
    void sleepMicroseconds(std::uint32_t usec) override { clock+=usec/1000; }
    int systemProperty(int code) const override { return code==3?1:0; }
    void updateModifiers() override {}
    void setPriority(int value) override { state.priority=value; }
    void setCursorMode(CursorMode) override;
    void warpCursor(int,int) override;
    bool pollJoystick(JoystickSample& sample) override { if (!state.joystickAvailable) return false; sample=joystick; return true; }
    void setJoystickTimer(unsigned value) override { timerInterval=value; }
};
}
