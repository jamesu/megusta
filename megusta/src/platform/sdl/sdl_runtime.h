#pragma once
#include "../runtime.h"
#ifdef MEGUSTA_SDL
struct SDL_Window;
struct _SDL_Joystick;
namespace megusta::platform {
class SdlRuntime:public Runtime {
public:
    SdlRuntime()=default;
    ~SdlRuntime() override;
    bool create(const char*) override;
    Capabilities capabilities() const override { return {false,true,true,false,true}; }
    bool pumpEvents() override;
    void finishAndPresent() override;
    void shutdown() override;
    std::uint32_t milliseconds() const override;
    void sleepMicroseconds(std::uint32_t) override;
    int systemProperty(int code) const override { return code==3 || code==4?1:0; }
    void updateModifiers() override;
    void setPriority(int value) override { state.priority=value; } // Browser has no process-priority API.
    void setCursorMode(CursorMode) override;
    void warpCursor(int,int) override; // Virtual cursor; browsers forbid moving the OS pointer.
    bool pollJoystick(JoystickSample&) override;
    void setJoystickTimer(unsigned value) override { timerInterval=value; }
    void runMainLoop() override;
    void tick();
protected:
    virtual void configureContext();
    virtual void contextCreated() {}
    virtual void contextClosing() {}
    virtual void stopMainLoop() {}
    SDL_Window* window=nullptr;
    _SDL_Joystick* joystick=nullptr;
    void* context=nullptr;
    bool running=false;
    bool initialized=false,sleepPending=false;
    std::uint32_t resumeAt=0;
    unsigned timerInterval=0;
    std::uint32_t lastJoystickTick=0;
    void reportError();
};
}
#endif
