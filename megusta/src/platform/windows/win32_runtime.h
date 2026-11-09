#pragma once
#include "../runtime.h"
#ifdef _WIN32
namespace megusta::platform {
class Win32Runtime final:public Runtime {
public:
    Win32Runtime(); ~Win32Runtime() override;
    bool create(const char*) override;
    void initializeFromArgs(int argc,char* const* argv) override;
    Capabilities capabilities() const override { return {true,true,true,true,true}; }
    void* nativeWindowHandle() const override;
    bool pumpEvents() override;
    void finishAndPresent() override;
    void shutdown() override;
    std::uint32_t milliseconds() const override;
    void sleepMicroseconds(std::uint32_t) override;
    int systemProperty(int) const override;
    void updateModifiers() override;
    void setPriority(int) override;
    void setCursorMode(CursorMode) override;
    void warpCursor(int,int) override;
    bool pollJoystick(JoystickSample&) override;
    void setJoystickTimer(unsigned) override;
    bool initializeJoystick();
    int run();
    void runMainLoop() override { run(); }
    // Native handles remain confined to this adapter, not game class ABIs.
    struct Native;
private:
    bool activateExistingInstance();
    void registerWindowClass();
    bool selectDisplayMode(int width,int height,int bits);
    Native* native;
};
}
#endif
