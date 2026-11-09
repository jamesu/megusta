#include "runtime.h"
#include <cstdlib>
#include <cstring>
namespace megusta::platform {
namespace {
char lower(char c) { return c>='A' && c<='Z'?char(c+('a'-'A')):c; }
bool match(const char* text,const char* key,bool bounded=false) {
    while (*key) if (lower(*text++)!=lower(*key++)) return false;
    return bounded || *text==0;
}
}
// 004d8780 shared argument branch. Original argc/argv are not modified.
void Runtime::initializeFromArgs(int argc,char* const* argv) {
    for (int i=1;i<argc;++i) {
        const char* arg=argv[i];
        if (match(arg,"-full")) { state.fullscreenRequested=true; state.fullscreenForbidden=true; }
        else if (match(arg,"-window")) { state.fullscreenRequested=false; state.fullscreenForbidden=true; }
        else if (match(arg,"-fullscreen:borderless")) state.fullscreenMode=FullscreenMode::Borderless;
        else if (match(arg,"-fullscreen:exclusive")) state.fullscreenMode=FullscreenMode::Exclusive;
        else if (match(arg,"-bitdepth:",true)) {
            int bits=std::atoi(arg+10);
            if (bits>15 && bits<33 && bits%8==0) state.colorBits=bits;
        } else if (match(arg,"-multi")) state.allowMultipleInstances=true;
    }
    if (!state.argumentsInitialized) {
        state.argumentsInitialized=true;
        const char* name=argv[0];
        const char* slash=std::strrchr(name,'\\');
        if (!slash) slash=std::strrchr(name,'/');
        if (slash) name=slash+1;
        const char* dot=std::strrchr(name,'.');
        state.className.assign(name,dot?static_cast<std::size_t>(dot-name):std::strlen(name));
    }
}
void Runtime::setInitialSize(int width,int height) { if (!state.created) { state.width=width; state.height=height; } }
void Runtime::requestInitialFullscreen() { if (!state.created && !state.fullscreenForbidden) state.fullscreenRequested=true; }
std::uint32_t Runtime::query(int code) const {
    if (code==0) return milliseconds();
    if (code==1) return static_cast<std::uint32_t>(state.width);
    if (code==2) return static_cast<std::uint32_t>(state.height);
    return 0;
}
int Runtime::activeModifiers() const { return state.created && state.modifiers!=-1?state.modifiers:0; }
void Runtime::registerJoystick(bool enabled,unsigned intervalMs) {
    joystickRequested=enabled;
    // The recovered helper stores or clears the callback whenever the window
    // subsystem is active. Device availability only affects polling; it does
    // not suppress callback dispatch (the original idle path still called it
    // with the retained input state when no DirectInput device was present).
    if (!state.created) return;
    joystickRegistered=enabled;
    if (intervalMs>0) setJoystickTimer(intervalMs);
}
void Runtime::applyJoystickSample(const JoystickSample& sample) {
    // Recovered from engineInput_pollDirectInputJoystick_impl: the device
    // button index is not the bit index. Keep this table in device order.
    static constexpr unsigned bits[16]={
        0x10,0x20,0x40,0x80,0x01,0x02,0x04,0x08,
        0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000
    };
    for (int i=0;i<16;++i) if (sample.buttons[i]&0x80) state.joystickButtons|=bits[i];
    for (int i=0;i<4;++i) {
        const float raw=static_cast<float>(static_cast<double>(sample.axes[i])*0.001);
        float squared=static_cast<float>(raw*raw);
        if (squared<0.1f) squared=0;
        if (raw<0) squared=-squared;
        state.joystickAxes[i]=squared;
    }
}
void Runtime::dispatchJoystick() {
    if (!joystickRegistered || !callbacks.joystick) return;
    JoystickSample sample;
    if (pollJoystick(sample)) applyJoystickSample(sample);
    callbacks.joystick(callbacks.context,state.joystickButtons,4,state.joystickAxes);
    state.joystickButtons=0;
}
void Runtime::oneFrame() {
    if (!state.minimized && callbacks.frame) { callbacks.frame(callbacks.context); finishAndPresent(); }
    else if (state.minimized) sleepMicroseconds(10000);
    if (callbacks.idle) callbacks.idle(callbacks.context);
}
void Runtime::runMainLoop() { while (pumpEvents()) oneFrame(); }
}
