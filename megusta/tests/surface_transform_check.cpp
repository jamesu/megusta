#include "platform/surface_transform.h"
#include <cassert>
#include <cstdio>
using megusta::platform::SurfaceTransform;
int main(){
    SurfaceTransform wide{1920,1080,1920,1080};
    auto r=wide.fit(640,480);assert(r.x==240 && r.y==0 && r.width==1440 && r.height==1080);
    int x=960,y=540;wide.toCanvas(x,y,640,480);assert(x==320 && y==240);
    wide.toWindow(x,y,640,480);assert(x==960 && y==540);
    // A configured 1024x768 game and a 640x480 menu share the physical
    // fitted rectangle but retain different logical input coordinates.
    const auto gameRect=wide.fit(1024,768);
    assert(gameRect.x==r.x && gameRect.y==r.y && gameRect.width==r.width && gameRect.height==r.height);
    x=960;y=540;wide.toCanvas(x,y,1024,768);assert(x==512 && y==384);
    wide.toWindow(x,y,1024,768);assert(x==960 && y==540);
    // Switching the logical viewport does not resize the native window.
    assert(wide.windowWidth==1920 && wide.windowHeight==1080);
    x=239;y=540;wide.toCanvas(x,y,640,480);assert(x<0);
    SurfaceTransform retina{1440,900,2880,1800};
    r=retina.fit(640,480);assert(r.x==240 && r.width==2400 && r.height==1800);
    x=720;y=450;retina.toCanvas(x,y,640,480);assert(x==320 && y==240);
    retina.toWindow(x,y,640,480);assert(x==720 && y==450);
    x=120;y=0;retina.toCanvas(x,y,640,480);assert(x==0 && y==0);
    SurfaceTransform portrait{1080,1920,1080,1920};
    r=portrait.fit(640,480);assert(r.y==555 && r.height==810);
    x=540;y=960;portrait.toCanvas(x,y,640,480);assert(x==320 && y==240);
    portrait.toWindow(x,y,640,480);assert(x==540 && y==960);
    SurfaceTransform odd{1601,900,1601,900};
    r=odd.fit(640,480);assert(r.x==200 && r.width==1200);
    const auto clip=wide.toPixels(0,0,640,480,640,480);
    assert(clip.x==240 && clip.width==1440 && clip.height==1080);
    const auto part=retina.toPixels(10,20,100,80,640,480);
    assert(part.x==277 && part.y==75 && part.width==376 && part.height==300);
    std::puts("PASS: 4:3 fit, resize, Retina, portrait, odd dimensions, input round trips and preview clipping");
}
