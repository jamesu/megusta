#include "browser_runtime.h"
#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <emscripten.h>
#include <gl4esinit.h>
namespace megusta::platform {
namespace {
void framebufferSize(int* w,int* h) { auto* window=SDL_GL_GetCurrentWindow();if(window)SDL_GL_GetDrawableSize(window,w,h);else {*w=0;*h=0;} }
}
void BrowserRuntime::configureContext() {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
}
void BrowserRuntime::contextCreated() {
    set_getprocaddress(SDL_GL_GetProcAddress);set_getmainfbsize(framebufferSize);
    initialize_gl4es();glInitialized=true;
}
void BrowserRuntime::contextClosing() { if(glInitialized){close_gl4es();glInitialized=false;} }
void BrowserRuntime::start() { loopStarted=true;emscripten_set_main_loop_arg([](void* p){static_cast<BrowserRuntime*>(p)->tick();},this,0,1); }
void BrowserRuntime::stopMainLoop() { if(loopStarted){emscripten_cancel_main_loop();loopStarted=false;} }
void BrowserRuntime::sleepMicroseconds(std::uint32_t usec) { resumeAt=milliseconds()+usec/1000;sleepPending=true; }
void BrowserRuntime::shutdown() { stopMainLoop();SdlRuntime::shutdown(); }
}
#endif
