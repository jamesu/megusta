#include "platform/sdl/pointer_state.h"
#include "platform/surface_transform.h"
#include <cassert>
#include <cstdio>
using namespace megusta::platform;
int main(){
    SdlPointerState pointer;
    pointer.motion(100,200,0,0,false);assert(pointer.x==100 && pointer.y==200);
    SurfaceTransform surface{1920,1080,1920,1080};
    int centerX=512,centerY=384;surface.toWindow(centerX,centerY,1024,768);
    pointer.reset(centerX,centerY);
    // SDL's absolute coordinate wraps or stays centered in relative mode.
    // Only the real relative deltas should reach the camera.
    pointer.motion(0,0,14,-7,true);
    assert(pointer.x==centerX+14 && pointer.y==centerY-7);
    int x=pointer.x,y=pointer.y;surface.toCanvas(x,y,1024,768);
    assert(x>512 && y<384);
    // A camera recenter must immediately replace the old edge position.
    pointer.reset(centerX,centerY);
    for(int tick=0;tick<20;++tick){
        x=pointer.x;y=pointer.y;surface.toCanvas(x,y,1024,768);
        assert(x==512 && y==384);
    }
    pointer.motion(1919,1079,0,0,true);assert(pointer.x==centerX && pointer.y==centerY);
    for(int i=0;i<10;++i)pointer.motion(0,0,2,1,true);
    assert(pointer.x==centerX+20 && pointer.y==centerY+10);
    // Absolute menu motion resumes after capture is released.
    pointer.motion(321,123,999,999,false);assert(pointer.x==321 && pointer.y==123);
    // Resize invalidation is followed by the camera's new recenter.
    pointer.valid=false;surface={1440,900,2880,1800};
    centerX=512;centerY=384;surface.toWindow(centerX,centerY,1024,768);
    pointer.reset(centerX,centerY);pointer.motion(0,0,0,0,true);
    x=pointer.x;y=pointer.y;surface.toCanvas(x,y,1024,768);
    assert(x==512 && y==384);
    std::puts("PASS: scaled relative motion, recenter without drift, absolute wrap rejection, menu release and Retina resize");
}
