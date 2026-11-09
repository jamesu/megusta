#include "hole_render_backend.h"
#include <GL/gl.h>
#include "ssg.h"
namespace megusta {
namespace {
OpenGLAdapter& adapter(void* p) { return *static_cast<OpenGLAdapter*>(p); }
void matrixMode(void* p,int v) { adapter(p).matrixMode(v); }
void loadIdentity(void*) { glLoadIdentity(); }
void projectionIdentity(void*) { ssgGetCurrentContext()->loadProjectionMatrix(); }
void depthRange(void* p,double a,double b) { adapter(p).depthRange(static_cast<float>(a),static_cast<float>(b)); }
void colorMask(void* p,bool r,bool g,bool b,bool a) { adapter(p).colorMask(r,g,b,a); }
void disable(void* p,int v) { adapter(p).disable(v); }
void enable(void* p,int v) { adapter(p).enable(v); }
void polygonMode(void* p,int a,int b) { adapter(p).polygonMode(a,b); }
void frontFace(void* p,int v) { adapter(p).frontFace(v); }
void disableClientState(void* p,int v) { adapter(p).disableClientState(v); }
void blendFunc(void* p,int a,int b) { adapter(p).blendFunc(a,b); }
void texEnvi(void* p,int a,int b,int c) { adapter(p).texEnvi(a,b,c); }
void depthMask(void* p,bool v) { adapter(p).depthMask(v); }
void color4f(void* p,float r,float g,float b,float a) { adapter(p).color4f(r,g,b,a); }
void shadeModel(void* p,int v) { adapter(p).shadeModel(v); }
void clearColor(void* p,float r,float g,float b,float a) { adapter(p).clearColor(r,g,b,a); }
void clearFlags(void* p,int v) { adapter(p).clearFlags(v); }
void depthFunc(void* p,int v) { adapter(p).depthFunc(v); }
void viewport(void* p,int x,int y,int w,int h) { adapter(p).viewport(x,y,w,h); }
void ortho(void* p,double l,double r,double b,double t,double n,double f) {
    adapter(p).ortho(static_cast<float>(l),static_cast<float>(r),static_cast<float>(b),static_cast<float>(t),static_cast<float>(n),static_cast<float>(f));
}
void selectBaseTextureUnit(void* p) { adapter(p).selectBaseTextureUnit(); }
}
HoleRenderServices makeOpenGLHoleRenderServices(OpenGLAdapter& a) {
    return {&a,matrixMode,loadIdentity,projectionIdentity,depthRange,colorMask,disable,enable,polygonMode,
            frontFace,disableClientState,blendFunc,texEnvi,depthMask,color4f,shadeModel,clearColor,clearFlags,depthFunc,viewport,ortho,
            selectBaseTextureUnit};
}
}
