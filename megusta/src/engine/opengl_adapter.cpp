#include "opengl_adapter.h"
#include "gl_token_table.h"
#include "platform_gl.h"
#include <cstring>
#if defined(MEGUSTA_SDL) && !defined(__EMSCRIPTEN__)
#include <SDL.h>
#endif
namespace megusta {
OpenGLAdapter::OpenGLAdapter(const Win32GLTokenTable& table):tokens(table.values) {}
void OpenGLAdapter::shadeModel(int n) { glShadeModel(tokens[n]); }
void OpenGLAdapter::polygonMode(int a,int b) { glPolygonMode(tokens[a],tokens[b]); }
void OpenGLAdapter::depthFunc(int n) { glDepthFunc(tokens[n]); }
void OpenGLAdapter::colorMask(bool r,bool g,bool b,bool a) { glColorMask(r,g,b,a); }
void OpenGLAdapter::depthRange(float a,float b) { glDepthRange(a,b); }
void OpenGLAdapter::matrixMode(int n) { glMatrixMode(tokens[n]); }
void OpenGLAdapter::loadIdentity() { glLoadIdentity(); }
void OpenGLAdapter::flush() { glFlush(); }
void OpenGLAdapter::viewport(int x,int y,int width,int height) { glViewport(x,y,width,height); }
void OpenGLAdapter::ortho(float l,float r,float b,float t,float n,float f) { glOrtho(l,r,b,t,n,f); }
void OpenGLAdapter::clearFlags(int flags) {
    GLbitfield result=0;
    if (flags&1) result|=GL_DEPTH_BUFFER_BIT;
    if (flags&2) result|=GL_COLOR_BUFFER_BIT;
    glClear(result);
}
void OpenGLAdapter::clearColor(float r,float g,float b,float a) { glClearColor(r,g,b,a); }
void OpenGLAdapter::color4f(float r,float g,float b,float a) { glColor4f(r,g,b,a); }
void OpenGLAdapter::alphaFunc(int n,float value) { glAlphaFunc(tokens[n],value); }
void OpenGLAdapter::begin(int n) { glBegin(tokens[n]); }
void OpenGLAdapter::end() { glEnd(); }
void OpenGLAdapter::vertex2i(int x,int y) { glVertex2i(x,y); }
void OpenGLAdapter::vertex2f(float x,float y) { glVertex2f(x,y); }
void OpenGLAdapter::vertex3f(float x,float y,float z) { glVertex3f(x,y,z); }
void OpenGLAdapter::vertex3fv(const float* p) { glVertex3fv(p); }
void OpenGLAdapter::color3f(float r,float g,float b) { glColor3f(r,g,b); }
void OpenGLAdapter::color3fv(const float* p) { glColor3fv(p); }
void OpenGLAdapter::texCoord2f(float s,float t) { glTexCoord2f(s,t); }
void OpenGLAdapter::texCoord2fv(const float* p) { glTexCoord2fv(p); }
void OpenGLAdapter::normal3f(float x,float y,float z) { glNormal3f(x,y,z); }
void OpenGLAdapter::normal3fv(const float* p) { glNormal3fv(p); }
void OpenGLAdapter::pushClientAttrib(int n) { glPushClientAttrib(tokens[n]); }
void OpenGLAdapter::colorPointer(int size,int type,int stride,const void* data) { glColorPointer(size,tokens[type],stride,data); }
void OpenGLAdapter::normalPointer(int type,int stride,const void* data) { glNormalPointer(tokens[type],stride,data); }
void OpenGLAdapter::texCoordPointer(int size,int type,int stride,const void* data) { glTexCoordPointer(size,tokens[type],stride,data); }
void OpenGLAdapter::drawArrays(int mode,int first,int count) { glDrawArrays(tokens[mode],first,count); }
void OpenGLAdapter::loadMatrix(const float* m) { glLoadMatrixf(m); }
void OpenGLAdapter::multiplyMatrix(const float* m) { glMultMatrixf(m); }
void OpenGLAdapter::pushMatrix() { glPushMatrix(); }
void OpenGLAdapter::popMatrix() { glPopMatrix(); }
void OpenGLAdapter::rotate(float angle,float x,float y,float z) { glRotatef(angle,x,y,z); }
void OpenGLAdapter::translate(float x,float y,float z) { glTranslatef(x,y,z); }
void OpenGLAdapter::scale(float x,float y,float z) { glScalef(x,y,z); }
void OpenGLAdapter::frustum(float l,float r,float b,float t,float n,float f) { glFrustum(l,r,b,t,n,f); }
void OpenGLAdapter::texSubImage2D(int a,int level,int x,int y,int w,int h,int f,int t,const void* p) {
    glTexSubImage2D(tokens[a],level,x,y,w,h,tokens[f],tokens[t],p);
}
void OpenGLAdapter::texGenfv(int a,int b,const float* p) { glTexGenfv(tokens[a],tokens[b],p); }
void OpenGLAdapter::pixelStorei(int name,int value) { glPixelStorei(tokens[name],value); }
void OpenGLAdapter::readPixels(int x,int y,int w,int h,int f,int t,void* p) { glReadPixels(x,y,w,h,tokens[f],tokens[t],p); }
void OpenGLAdapter::getIntegerv(int n,int* p) { glGetIntegerv(tokens[n],p); }
void OpenGLAdapter::getFloatv(int n,float* p) { glGetFloatv(tokens[n],p); }
void OpenGLAdapter::lightfv(int a,int b,const float* p) { glLightfv(tokens[a],tokens[b],p); }
void OpenGLAdapter::materialf(int a,int b,float value) { glMaterialf(tokens[a],tokens[b],value); }
void OpenGLAdapter::colorMaterial(int a,int b) { glColorMaterial(tokens[a],tokens[b]); }
std::uint32_t OpenGLAdapter::generateLists(int count) { return glGenLists(count); }
void OpenGLAdapter::newList(std::uint32_t list,int mode) { glNewList(list,tokens[mode]); }
void OpenGLAdapter::endList() { glEndList(); }
void OpenGLAdapter::callList(std::uint32_t list) { glCallList(list); }
void OpenGLAdapter::deleteLists(std::uint32_t list,int count) { glDeleteLists(list,count); }
void OpenGLAdapter::scissor(int x,int y,int w,int h) { glScissor(x,y,w,h); }
void OpenGLAdapter::loadMultitextureFunctions() {
#ifdef __EMSCRIPTEN__
    // All other browser GL calls pass through gl4es. Resolve these through
    // the same header aliases so gl4es's cached active-unit state stays in
    // sync; SDL_GL_GetProcAddress would return the underlying GLES entrypoint.
    activeTextureFunction=reinterpret_cast<GLActiveTextureProc>(&glActiveTexture);
    clientActiveTextureFunction=reinterpret_cast<GLActiveTextureProc>(&glClientActiveTexture);
#elif defined(_WIN32)
    activeTextureFunction=reinterpret_cast<GLActiveTextureProc>(wglGetProcAddress("glActiveTextureARB"));
    if (!activeTextureFunction)
        activeTextureFunction=reinterpret_cast<GLActiveTextureProc>(wglGetProcAddress("glActiveTexture"));
    clientActiveTextureFunction=reinterpret_cast<GLActiveTextureProc>(wglGetProcAddress("glClientActiveTextureARB"));
#elif defined(MEGUSTA_SDL)
    activeTextureFunction=reinterpret_cast<GLActiveTextureProc>(SDL_GL_GetProcAddress("glActiveTexture"));
    clientActiveTextureFunction=reinterpret_cast<GLActiveTextureProc>(SDL_GL_GetProcAddress("glClientActiveTexture"));
#endif
}
}
