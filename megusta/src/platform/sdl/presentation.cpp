#include "presentation.h"
#include "../../engine/platform_gl.h"
namespace megusta::platform {
void beginWorldOverlay(const SurfaceTransform& surface,int width,int height) {
    const auto gui=surface.fit(width,height);
    glPushAttrib(GL_VIEWPORT_BIT|GL_SCISSOR_BIT|GL_ENABLE_BIT|GL_TRANSFORM_BIT|GL_COLOR_BUFFER_BIT);
    glViewport(0,0,surface.drawableWidth,surface.drawableHeight);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
    glOrtho(-double(gui.x)*width/gui.width,double(surface.drawableWidth-gui.x)*width/gui.width,
        -double(gui.y)*height/gui.height,double(surface.drawableHeight-gui.y)*height/gui.height,0,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
}
void endWorldOverlay() {
    glMatrixMode(GL_MODELVIEW);glPopMatrix();
    glMatrixMode(GL_PROJECTION);glPopMatrix();glPopAttrib();
}
void clearLetterboxBars(const SurfaceTransform& surface,int width,int height) {
    if(!surface.enabled())return;
    const auto r=surface.fit(width,height);
    if(r.width==surface.drawableWidth && r.height==surface.drawableHeight)return;
    glPushAttrib(GL_SCISSOR_BIT|GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(0,0,0,1);
    const auto clear=[](int x,int y,int w,int h){if(w>0 && h>0){glScissor(x,y,w,h);glClear(GL_COLOR_BUFFER_BIT);}};
    clear(0,0,r.x,surface.drawableHeight);
    clear(r.x+r.width,0,surface.drawableWidth-r.x-r.width,surface.drawableHeight);
    clear(r.x,0,r.width,r.y);
    clear(r.x,r.y+r.height,r.width,surface.drawableHeight-r.y-r.height);
    glPopAttrib();
}
}
