#include "opengl_adapter.h"
#include "gl_token_table.h"
#include <windows.h>
#include <GL/gl.h>
#include <cstring>
namespace megusta {
OpenGLAdapter::OpenGLAdapter(const Win32GLTokenTable& table):tokens(table.values) {}
void OpenGLAdapter::enable(int n) { glEnable(tokens[n]); }
void OpenGLAdapter::disable(int n) { glDisable(tokens[n]); }
void OpenGLAdapter::shadeModel(int n) { glShadeModel(tokens[n]); }
void OpenGLAdapter::polygonMode(int a,int b) { glPolygonMode(tokens[a],tokens[b]); }
void OpenGLAdapter::blendFunc(int a,int b) { glBlendFunc(tokens[a],tokens[b]); }
void OpenGLAdapter::depthFunc(int n) { glDepthFunc(tokens[n]); }
void OpenGLAdapter::depthMask(bool n) { glDepthMask(n); }
void OpenGLAdapter::colorMask(bool r,bool g,bool b,bool a) { glColorMask(r,g,b,a); }
void OpenGLAdapter::depthRange(float a,float b) { glDepthRange(a,b); }
void OpenGLAdapter::disableClientState(int n) { glDisableClientState(tokens[n]); }
void OpenGLAdapter::matrixMode(int n) { glMatrixMode(tokens[n]); }
void OpenGLAdapter::loadIdentity() { glLoadIdentity(); }
void OpenGLAdapter::flush() { glFlush(); }
void OpenGLAdapter::viewport(int x,int y,int width,int height) { glViewport(x,y,width,height); }
void OpenGLAdapter::ortho(float l,float r,float b,float t,float n,float f) { glOrtho(l,r,b,t,n,f); }
void OpenGLAdapter::clearFlags(int flags) {
    GLbitfield result=0; if (flags&1) result|=GL_DEPTH_BUFFER_BIT; if (flags&2) result|=GL_COLOR_BUFFER_BIT; glClear(result);
}
void OpenGLAdapter::clearColor(float r,float g,float b,float a) { glClearColor(r,g,b,a); }
void OpenGLAdapter::texEnvi(int a,int b,int c) { glTexEnvi(tokens[a],tokens[b],tokens[c]); }
void OpenGLAdapter::frontFace(int n) { glFrontFace(tokens[n]); }
void OpenGLAdapter::color4f(float r,float g,float b,float a) { glColor4f(r,g,b,a); }
bool OpenGLAdapter::isEnabled(int n) { return glIsEnabled(tokens[n])!=0; }
void OpenGLAdapter::polygonOffset(float a,float b) { glPolygonOffset(a,b); }
void OpenGLAdapter::alphaFunc(int n,float value) { glAlphaFunc(tokens[n],value); }
void OpenGLAdapter::begin(int n) { glBegin(tokens[n]); }
void OpenGLAdapter::end() { glEnd(); }
void OpenGLAdapter::vertex2i(int x,int y) { glVertex2i(x,y); }
void OpenGLAdapter::vertex2f(float x,float y) { glVertex2f(x,y); }
void OpenGLAdapter::vertex3f(float x,float y,float z) { glVertex3f(x,y,z); }
void OpenGLAdapter::vertex3fv(const float* p) { glVertex3fv(p); }
void OpenGLAdapter::color3f(float r,float g,float b) { glColor3f(r,g,b); }
void OpenGLAdapter::color3fv(const float* p) { glColor3fv(p); }
void OpenGLAdapter::color4fv(const float* p) { glColor4fv(p); }
void OpenGLAdapter::texCoord2f(float s,float t) { glTexCoord2f(s,t); }
void OpenGLAdapter::texCoord2fv(const float* p) { glTexCoord2fv(p); }
void OpenGLAdapter::normal3f(float x,float y,float z) { glNormal3f(x,y,z); }
void OpenGLAdapter::normal3fv(const float* p) { glNormal3fv(p); }
void OpenGLAdapter::pushClientAttrib(int n) { glPushClientAttrib(tokens[n]); }
void OpenGLAdapter::enableClientState(int n) { glEnableClientState(tokens[n]); }
void OpenGLAdapter::vertexPointer(int size,int type,int stride,const void* data) { glVertexPointer(size,tokens[type],stride,data); }
void OpenGLAdapter::colorPointer(int size,int type,int stride,const void* data) { glColorPointer(size,tokens[type],stride,data); }
void OpenGLAdapter::normalPointer(int type,int stride,const void* data) { glNormalPointer(tokens[type],stride,data); }
void OpenGLAdapter::texCoordPointer(int size,int type,int stride,const void* data) { glTexCoordPointer(size,tokens[type],stride,data); }
void OpenGLAdapter::drawElements(int mode,int count,int type,const void* indices) { glDrawElements(tokens[mode],count,tokens[type],indices); }
void OpenGLAdapter::drawArrays(int mode,int first,int count) { glDrawArrays(tokens[mode],first,count); }
void OpenGLAdapter::loadMatrix(const float* m) { glLoadMatrixf(m); }
void OpenGLAdapter::multiplyMatrix(const float* m) { glMultMatrixf(m); }
void OpenGLAdapter::pushMatrix() { glPushMatrix(); }
void OpenGLAdapter::popMatrix() { glPopMatrix(); }
void OpenGLAdapter::rotate(float angle,float x,float y,float z) { glRotatef(angle,x,y,z); }
void OpenGLAdapter::translate(float x,float y,float z) { glTranslatef(x,y,z); }
void OpenGLAdapter::scale(float x,float y,float z) { glScalef(x,y,z); }
void OpenGLAdapter::frustum(float l,float r,float b,float t,float n,float f) { glFrustum(l,r,b,t,n,f); }
void OpenGLAdapter::generateTextures(int count,std::uint32_t* result) { glGenTextures(count,result); }
void OpenGLAdapter::deleteTextures(int count,const std::uint32_t* values) { glDeleteTextures(count,values); }
void OpenGLAdapter::bindTexture(int target,std::uint32_t texture) { glBindTexture(tokens[target],texture); }
void OpenGLAdapter::textureImage2D(int target,int level,int internalFormat,int width,int height,int border,
                                   int format,int type,const void* data,int textureDepth) {
    if (internalFormat<1 || internalFormat>4) internalFormat=static_cast<int>(tokens[internalFormat]);
    if (textureDepth==16) {
        if (internalFormat==GL_RGB || internalFormat==3) internalFormat=0x8050;
        else if (internalFormat==GL_RGBA || internalFormat==4) internalFormat=0x8056;
    }
    glTexImage2D(tokens[target],level,internalFormat,width,height,border,tokens[format],tokens[type],data);
}
void OpenGLAdapter::texEnvfv(int a,int b,const float* p) { glTexEnvfv(tokens[a],tokens[b],p); }
void OpenGLAdapter::texParameteri(int a,int b,int c) { glTexParameteri(tokens[a],tokens[b],tokens[c]); }
void OpenGLAdapter::texSubImage2D(int a,int level,int x,int y,int w,int h,int f,int t,const void* p) { glTexSubImage2D(tokens[a],level,x,y,w,h,tokens[f],tokens[t],p); }
void OpenGLAdapter::texGeni(int a,int b,int c) { glTexGeni(tokens[a],tokens[b],tokens[c]); }
void OpenGLAdapter::texGenfv(int a,int b,const float* p) { glTexGenfv(tokens[a],tokens[b],p); }
void OpenGLAdapter::pixelStorei(int name,int value) { glPixelStorei(tokens[name],value); }
void OpenGLAdapter::readPixels(int x,int y,int w,int h,int f,int t,void* p) { glReadPixels(x,y,w,h,tokens[f],tokens[t],p); }
void OpenGLAdapter::getIntegerv(int n,int* p) { glGetIntegerv(tokens[n],p); }
void OpenGLAdapter::getFloatv(int n,float* p) { glGetFloatv(tokens[n],p); }
void OpenGLAdapter::lightfv(int a,int b,const float* p) { glLightfv(tokens[a],tokens[b],p); }
void OpenGLAdapter::materialf(int a,int b,float value) { glMaterialf(tokens[a],tokens[b],value); }
void OpenGLAdapter::materialfv(int a,int b,const float* p) { glMaterialfv(tokens[a],tokens[b],p); }
void OpenGLAdapter::colorMaterial(int a,int b) { glColorMaterial(tokens[a],tokens[b]); }
std::uint32_t OpenGLAdapter::generateLists(int count) { return glGenLists(count); }
void OpenGLAdapter::newList(std::uint32_t list,int mode) { glNewList(list,tokens[mode]); }
void OpenGLAdapter::endList() { glEndList(); }
void OpenGLAdapter::callList(std::uint32_t list) { glCallList(list); }
void OpenGLAdapter::deleteLists(std::uint32_t list,int count) { glDeleteLists(list,count); }
const char* OpenGLAdapter::getStringOrPlatform(int n) { return n==0xa4 ? "win32" : reinterpret_cast<const char*>(glGetString(tokens[n])); }
void OpenGLAdapter::scissor(int x,int y,int w,int h) { glScissor(x,y,w,h); }
void OpenGLAdapter::loadMultitextureFunctions() {
    const char* extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions && std::strstr(extensions,"GL_ARB_multitexture")) {
        activeTextureFunction=reinterpret_cast<GLActiveTextureProc>(wglGetProcAddress("glActiveTextureARB"));
        clientActiveTextureFunction=reinterpret_cast<GLActiveTextureProc>(wglGetProcAddress("glClientActiveTextureARB"));
    }
}
void OpenGLAdapter::activeTexture(int texture) { activeTextureFunction(tokens[texture]); }
void OpenGLAdapter::clientActiveTexture(int texture) { clientActiveTextureFunction(tokens[texture]); }
}
