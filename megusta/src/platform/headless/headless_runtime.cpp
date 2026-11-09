#include "../runtime.h"
namespace megusta::platform {
bool HeadlessRuntime::create(const char*) { state.created=state.active=running=true; return true; }
void HeadlessRuntime::shutdown() { running=false; state.created=state.active=state.cursorCaptured=false; }
void HeadlessRuntime::setCursorMode(CursorMode mode) {
    if (state.active && (mode==CursorMode::Released || mode==CursorMode::Captured)) state.cursorCaptured=mode==CursorMode::Captured;
}
void HeadlessRuntime::warpCursor(int x,int y) { if (state.active) { cursorX=x; cursorY=y; } }
}
