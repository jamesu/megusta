#include "opengl_adapter.h"
#include "gl_token_table.h"
#include <GL/gl.h>
namespace megusta {
void OpenGLAdapter::enable(int n) { glEnable(tokens[n]); }
void OpenGLAdapter::disable(int n) { glDisable(tokens[n]); }
void OpenGLAdapter::blendFunc(int a,int b) { glBlendFunc(tokens[a],tokens[b]); }
void OpenGLAdapter::depthMask(bool n) { glDepthMask(n); }
void OpenGLAdapter::disableClientState(int n) { glDisableClientState(tokens[n]); }
void OpenGLAdapter::texEnvi(int a,int b,int c) { glTexEnvi(tokens[a],tokens[b],tokens[c]); }
void OpenGLAdapter::frontFace(int n) { glFrontFace(tokens[n]); }
void OpenGLAdapter::polygonOffset(float a,float b) { glPolygonOffset(a,b); }
void OpenGLAdapter::color4fv(const float* p) { glColor4fv(p); }
void OpenGLAdapter::enableClientState(int n) { glEnableClientState(tokens[n]); }
void OpenGLAdapter::vertexPointer(int size,int type,int stride,const void* data) {
    glVertexPointer(size,tokens[type],stride,data);
}
void OpenGLAdapter::drawElements(int mode,int count,int type,const void* indices) {
    glDrawElements(tokens[mode],count,tokens[type],indices);
}
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
void OpenGLAdapter::texGeni(int a,int b,int c) { glTexGeni(tokens[a],tokens[b],tokens[c]); }
void OpenGLAdapter::materialfv(int a,int b,const float* p) { glMaterialfv(tokens[a],tokens[b],p); }
const char* OpenGLAdapter::getStringOrPlatform(int n) {
    return n==0xa4 ? "win32" : reinterpret_cast<const char*>(glGetString(tokens[n]));
}
void OpenGLAdapter::activeTexture(int texture) { activeTextureFunction(tokens[texture]); }
void OpenGLAdapter::clientActiveTexture(int texture) { clientActiveTextureFunction(tokens[texture]); }
}
