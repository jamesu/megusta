#include "platform/sdl/sdl_runtime.h"
#include "platform/surface_transform.h"
#include <cassert>
#include <cstdio>
using namespace megusta::platform;
struct RuntimeProbe: SdlRuntime {
    const SdlPointerState& position()const{return pointer;}
};
struct InputProbe { SurfaceTransform surface{1920,1080,1920,1080};int x=0,y=0,calls=0; };
int main(){
    RuntimeProbe runtime;InputProbe input;
    runtime.callbacks.context=&input;
    runtime.callbacks.passiveMotion=[](void* p,int x,int y){
        auto& input=*static_cast<InputProbe*>(p);input.surface.toCanvas(x,y,1024,768);
        input.x=x;input.y=y;++input.calls;
    };
    // Fullscreen focus transitions can leave the platform inactive while
    // camera ticks continue. The virtual recenter must still reach input.
    runtime.state.active=false;
    int x=512,y=384;input.surface.toWindow(x,y,1024,768);
    runtime.warpCursor(x,y);
    assert(input.x==512 && input.y==384 && input.calls==1);
    assert(runtime.position().x==x && runtime.position().y==y);
    runtime.warpCursor(x,y);assert(input.x==512 && input.y==384 && input.calls==2);
    std::puts("PASS: actual SDL runtime publishes scaled camera recenters synchronously while inactive; no window required");
}
