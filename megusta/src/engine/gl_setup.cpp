#include "gl_setup.h"
#include "opengl_adapter.h"
#include <GL/gl.h>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
namespace megusta {
namespace {
void* resolve(const char* name,void* (*host)(const char*)) {
    if (host) return host(name);
#ifdef _WIN32
    return reinterpret_cast<void*>(wglGetProcAddress(name));
#else
    throw std::runtime_error("GL extension procedure resolver is not bound for this platform");
#endif
}
}
void initializeExtensionsAndSwapInterval(OpenGLAdapter& gl,GLSetupState& state,
        int savedArgumentA,int savedArgumentB) {
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);
    state.lastHintTarget=savedArgumentA; state.lastHintMode=savedArgumentB;
    if (state.hintHook) state.hintHook(state.lastHintTarget,state.lastHintMode);
    gl.loadMultitextureFunctions();
    if (state.swapInterval<0) return;
    const char* extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions && std::strstr(extensions,"WGL_EXT_swap_control")) {
        using SwapInterval=int (APIENTRY*)(int);
        if (auto function=reinterpret_cast<SwapInterval>(resolve("wglSwapIntervalEXT",state.resolveProcedure))) function(state.swapInterval!=0);
    }
}
bool uploadPalettedMipChain(PalettedTextureState& state,int width,int height,int levels,
        const std::uint8_t* pixels,const std::uint8_t* palette,int firstMip) {
    if (state.colorBits>8) return false;
    if (state.supported==-1) {
        const auto* extensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        if (extensions && std::strstr(extensions,"GL_EXT_paletted_texture")) {
            const auto* version=reinterpret_cast<const char*>(glGetString(GL_VERSION));
            if (!std::strstr(version,"1.1.0 3Dfx Beta")) state.colorTableProcedure=resolve("glColorTableEXT",state.resolveProcedure);
        }
        state.supported=state.colorTableProcedure!=nullptr;
    }
    if (!state.supported) return false;
    if (state.maximumTextureSize==-1) glGetIntegerv(GL_MAX_TEXTURE_SIZE,&state.maximumTextureSize);
    if (levels==1 && (width>state.maximumTextureSize || height>state.maximumTextureSize)) return false;
    GLint alignment; glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    using ColorTable=void (APIENTRY*)(GLenum,GLenum,GLsizei,GLenum,GLenum,const void*);
    reinterpret_cast<ColorTable>(state.colorTableProcedure)(GL_TEXTURE_2D,GL_RGBA,256,GL_RGBA,GL_UNSIGNED_BYTE,palette);
    int output=0;
    for (int mip=0;mip<levels;++mip) {
        if (width<=state.maximumTextureSize && height<=state.maximumTextureSize && (firstMip<=mip || mip==levels-1))
            glTexImage2D(GL_TEXTURE_2D,output++,0x80e5,width,height,0,GL_COLOR_INDEX,GL_UNSIGNED_BYTE,pixels);
        pixels+=width*height; if (width==1 && height==1) break; if (width>1) width/=2; if (height>1) height/=2;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment); return true;
}
}
