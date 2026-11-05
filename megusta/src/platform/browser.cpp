#include "browser.h"
#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <GLES2/gl2.h>
namespace megusta::platform {
namespace {
constexpr unsigned subsystems=SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK|SDL_INIT_TIMER;
int special(SDL_Keycode key) {
    if (key>=SDLK_F1 && key<=SDLK_F12) return static_cast<int>(key-SDLK_F1)+1;
    switch (key) {
    case SDLK_LEFT:return 100; case SDLK_UP:return 101; case SDLK_RIGHT:return 102; case SDLK_DOWN:return 103;
    case SDLK_PAGEUP:return 104; case SDLK_PAGEDOWN:return 105; case SDLK_HOME:return 106;
    case SDLK_END:return 107; case SDLK_INSERT:return 108; default:return 0;
    }
}
}
BrowserRuntime::~BrowserRuntime() { shutdown(); }
void BrowserRuntime::reportError() { if (callbacks.error) callbacks.error(callbacks.context,SDL_GetError()); }
bool BrowserRuntime::create(const char* title) {
    if (state.created || state.width<=0 || state.height<=0) return false;
    if (SDL_InitSubSystem(subsystems)!=0) { reportError(); return false; }
    initialized=true;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
    unsigned flags=SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE;
    window=SDL_CreateWindow(title,SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,state.width,state.height,flags);
    if (!window) { reportError(); shutdown(); return false; }
    context=SDL_GL_CreateContext(window);
    if (!context || SDL_GL_MakeCurrent(window,context)!=0) { reportError(); shutdown(); return false; }
    SDL_StartTextInput();
    state.created=state.active=running=true;
    // SDL's Emscripten port owns browser gesture/deferred-fullscreen handling.
    if (state.fullscreenRequested) {
        const unsigned mode=state.fullscreenMode==FullscreenMode::Borderless?
            SDL_WINDOW_FULLSCREEN_DESKTOP:SDL_WINDOW_FULLSCREEN;
        if (SDL_SetWindowFullscreen(window,mode)!=0) reportError();
    }
    return true;
}
std::uint32_t BrowserRuntime::milliseconds() const { return SDL_GetTicks(); }
void BrowserRuntime::sleepMicroseconds(std::uint32_t usec) { resumeAt=milliseconds()+usec/1000; sleepPending=true; }
void BrowserRuntime::updateModifiers() {
    const auto mods=SDL_GetModState(); state.modifiers=0;
    if (mods&KMOD_SHIFT) state.modifiers|=Shift;
    if (mods&KMOD_CTRL) state.modifiers|=Control;
    if (mods&KMOD_ALT) state.modifiers|=Alt;
}
bool BrowserRuntime::pumpEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        auto& cb=callbacks; updateModifiers();
        switch (event.type) {
        case SDL_QUIT: running=false; break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: state.width=event.window.data1; state.height=event.window.data2; break;
            case SDL_WINDOWEVENT_FOCUS_GAINED: state.active=true; break;
            case SDL_WINDOWEVENT_FOCUS_LOST: state.active=false; state.modifiers=0; break;
            case SDL_WINDOWEVENT_MINIMIZED: state.minimized=true; break;
            case SDL_WINDOWEVENT_RESTORED: state.minimized=false; break;
            case SDL_WINDOWEVENT_CLOSE: running=false; break;
            default: break;
            }
            break;
        case SDL_KEYDOWN: case SDL_KEYUP: {
            const int code=special(event.key.keysym.sym);
            if (code) { auto callback=event.type==SDL_KEYUP?cb.specialUp:cb.specialDown; if (callback) callback(cb.context,code); }
            else if (event.type==SDL_KEYDOWN) {
                const auto key=event.key.keysym.sym;
                if (key==SDLK_ESCAPE && state.exitOnEscape) running=false;
                if ((key==SDLK_RETURN || key==SDLK_BACKSPACE || key==SDLK_TAB || key==SDLK_ESCAPE || key==SDLK_DELETE) && cb.keyDown)
                    cb.keyDown(cb.context,static_cast<std::uint8_t>(key));
            }
            break;
        }
        case SDL_TEXTINPUT:
            // The recovered game callback accepts an 8-bit character. ASCII
            // text is supported; a legacy encoding policy for Unicode is pending.
            if (cb.keyDown) for (const unsigned char* p=reinterpret_cast<unsigned char*>(event.text.text);*p;++p)
                if (*p<128) cb.keyDown(cb.context,*p);
            break;
        case SDL_MOUSEMOTION: {
            auto callback=event.motion.state?cb.mouseMotion:cb.passiveMotion;
            if (callback) callback(cb.context,event.motion.x,event.motion.y); break;
        }
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: {
            int button=event.button.button==SDL_BUTTON_LEFT?0:event.button.button==SDL_BUTTON_MIDDLE?1:event.button.button==SDL_BUTTON_RIGHT?2:-1;
            if (button>=0 && cb.mouseButton) cb.mouseButton(cb.context,button,event.type==SDL_MOUSEBUTTONDOWN?0:1,event.button.x,event.button.y);
            break;
        }
        default: break;
        }
    }
    state.cursorCaptured=SDL_GetRelativeMouseMode()==SDL_TRUE;
    return running;
}
void BrowserRuntime::finishAndPresent() { glFinish(); SDL_GL_SwapWindow(window); }
void BrowserRuntime::setCursorMode(CursorMode mode) {
    if (!state.active || (mode!=CursorMode::Released && mode!=CursorMode::Captured)) return;
    if (SDL_SetRelativeMouseMode(mode==CursorMode::Captured?SDL_TRUE:SDL_FALSE)!=0) reportError();
    state.cursorCaptured=SDL_GetRelativeMouseMode()==SDL_TRUE;
}
void BrowserRuntime::warpCursor(int x,int y) { if (state.active) SDL_WarpMouseInWindow(window,x,y); }
bool BrowserRuntime::pollJoystick(JoystickSample& sample) {
    if (joystick && !SDL_JoystickGetAttached(joystick)) { SDL_JoystickClose(joystick); joystick=nullptr; }
    if (!joystick && SDL_NumJoysticks()>0) joystick=SDL_JoystickOpen(0);
    state.joystickAvailable=joystick!=nullptr;
    if (!joystick) return false;
    SDL_JoystickUpdate();
    for (int i=0;i<4;++i) {
        const int raw=i<SDL_JoystickNumAxes(joystick)?SDL_JoystickGetAxis(joystick,i):0;
        sample.axes[i]=raw>=0?raw*1000/32767:raw*1000/32768;
    }
    for (int i=0;i<32;++i) sample.buttons[i]=i<SDL_JoystickNumButtons(joystick) && SDL_JoystickGetButton(joystick,i)?0x80:0;
    return true;
}
void BrowserRuntime::tick() {
    if (!pumpEvents()) { shutdown(); return; }
    if (sleepPending) { if (static_cast<std::int32_t>(milliseconds()-resumeAt)<0) return; sleepPending=false; }
    JoystickSample sample; pollJoystick(sample);
    if (state.joystickAvailable) registerJoystick(joystickRequested);
    if (timerInterval && milliseconds()-lastJoystickTick>=timerInterval) { lastJoystickTick=milliseconds(); dispatchJoystick(); }
    oneFrame();
}
void BrowserRuntime::start() { loopStarted=true; emscripten_set_main_loop_arg([](void* p){static_cast<BrowserRuntime*>(p)->tick();},this,0,1); }
void BrowserRuntime::shutdown() {
    running=false;
    if (loopStarted) { emscripten_cancel_main_loop(); loopStarted=false; }
    if (joystick) { SDL_JoystickClose(joystick); joystick=nullptr; }
    if (context) { SDL_GL_DeleteContext(context); context=nullptr; }
    if (window) { SDL_DestroyWindow(window); window=nullptr; }
    if (initialized) { SDL_StopTextInput(); SDL_QuitSubSystem(subsystems); initialized=false; }
    state.created=state.active=state.cursorCaptured=state.joystickAvailable=false;
}
}
#endif
