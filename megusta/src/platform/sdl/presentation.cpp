#include "presentation.h"
#include <GL/gl.h>
namespace megusta::platform {
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
