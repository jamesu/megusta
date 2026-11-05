#pragma once
#include <cstdint>
namespace megusta {
class Win32GLTokenTable;
#if defined(_WIN32)
using GLActiveTextureProc = void (__stdcall *)(std::uint32_t);
#else
using GLActiveTextureProc = void (*)(std::uint32_t);
#endif
// Token values must come from the recovered engine table; the adapter does not
// guess missing entries. The original also indexes the table without checks.
class OpenGLAdapter {
public:
    explicit OpenGLAdapter(const std::uint32_t* engineTokens):tokens(engineTokens) {}
    explicit OpenGLAdapter(const Win32GLTokenTable& engineTokens);
    const std::uint32_t* tokens;
    GLActiveTextureProc activeTextureFunction=nullptr,clientActiveTextureFunction=nullptr;
    void enable(int token);
    void disable(int token);
    void shadeModel(int token);
    void polygonMode(int face,int mode);
    void blendFunc(int source,int destination);
    void depthFunc(int token);
    void depthMask(bool value);
    void colorMask(bool r,bool g,bool b,bool a);
    void depthRange(float nearValue,float farValue);
    void disableClientState(int token);
    void matrixMode(int token);
    void viewport(int x,int y,int width,int height);
    void ortho(float left,float right,float bottom,float top,float nearValue,float farValue);
    void clearFlags(int flags);
    void clearColor(float r,float g,float b,float a);
    void texEnvi(int target,int name,int value);
    void frontFace(int token);
    void color4f(float r,float g,float b,float a);
    bool isEnabled(int token);
    void polygonOffset(float factor,float units);
    void alphaFunc(int token,float reference);
    void begin(int token);
    void end();
    void vertex2i(int x,int y);
    void vertex2f(float x,float y);
    void vertex3f(float x,float y,float z);
    void vertex3fv(const float* p);
    void color3f(float r,float g,float b);
    void color3fv(const float* p);
    void color4fv(const float* p);
    void texCoord2f(float s,float t);
    void texCoord2fv(const float* p);
    void normal3f(float x,float y,float z);
    void normal3fv(const float* p);
    void pushClientAttrib(int token);
    void enableClientState(int token);
    void vertexPointer(int size,int type,int stride,const void* data);
    void colorPointer(int size,int type,int stride,const void* data);
    void normalPointer(int type,int stride,const void* data);
    void texCoordPointer(int size,int type,int stride,const void* data);
    void drawElements(int mode,int count,int type,const void* indices);
    void drawArrays(int mode,int first,int count);
    void loadMatrix(const float* matrix);
    void loadIdentity();
    void flush();
    void multiplyMatrix(const float* matrix);
    void pushMatrix();
    void popMatrix();
    void rotate(float angle,float x,float y,float z);
    void translate(float x,float y,float z);
    void scale(float x,float y,float z);
    void frustum(float left,float right,float bottom,float top,float nearValue,float farValue);
    void generateTextures(int count,std::uint32_t* textures);
    void deleteTextures(int count,const std::uint32_t* textures);
    void bindTexture(int target,std::uint32_t texture);
    void textureImage2D(int target,int level,int internalFormat,int width,int height,int border,
                        int format,int type,const void* data,int textureDepth);
    void texEnvfv(int target,int name,const float* values);
    void texParameteri(int target,int name,int value);
    void texSubImage2D(int target,int level,int x,int y,int width,int height,int format,int type,const void* data);
    void texGeni(int coordinate,int name,int value);
    void texGenfv(int coordinate,int name,const float* values);
    void pixelStorei(int name,int value);
    void readPixels(int x,int y,int width,int height,int format,int type,void* pixels);
    void getIntegerv(int name,int* values);
    void getFloatv(int name,float* values);
    void lightfv(int light,int name,const float* values);
    void materialf(int face,int name,float value);
    void materialfv(int face,int name,const float* values);
    void colorMaterial(int face,int mode);
    std::uint32_t generateLists(int count);
    void newList(std::uint32_t list,int mode);
    void endList();
    void callList(std::uint32_t list);
    void deleteLists(std::uint32_t list,int count);
    const char* getStringOrPlatform(int name);
    void scissor(int x,int y,int width,int height);
    void loadMultitextureFunctions();
    void activeTexture(int texture);
    void clientActiveTexture(int texture);
    void emitNormalizedNormal(float x,float y,float z);
    void drawTorus(float innerRadius,int sides,float outerRadius,int rings);
    void drawCylinder(float baseRadius,float topRadius,float height,int slices,int stacks);
    void drawSphere(float radius,int slices,int stacks);
};
}
