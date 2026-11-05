#include "render_projection.h"
#include "opengl_adapter.h"
#include "dts/matrix.h"
namespace megusta {
void loadConvertedMatrix(OpenGLAdapter& gl,float* matrix) {
    dts::transposeMatrix4(matrix); gl.loadMatrix(matrix); dts::transposeMatrix4(matrix);
}
void multiplyConvertedMatrix(OpenGLAdapter& gl,float* matrix) {
    dts::transposeMatrix4(matrix); gl.multiplyMatrix(matrix); dts::transposeMatrix4(matrix);
}
void RenderProjectionState::setProjectionBounds(OpenGLAdapter& gl,float l,float r,float b,float t,float n,float f,bool ortho) {
    left=l; right=r; bottom=b; top=t; nearPlane=n; farPlane=f; orthographic=ortho;
    if (!ortho) { gl.frustum(l,r,b,t,n,f); pixelScale=static_cast<float>(viewportWidth)*nearPlane; }
    else { gl.ortho(l,r,b,t,n,f); pixelScale=static_cast<float>(viewportWidth); }
    pixelScale/=right-left; gl.multiplyMatrix(projectionAdjustment);
}
void RenderProjectionState::setViewport(OpenGLAdapter& gl,const int* rectangle,int displayHeight) {
    viewportX=rectangle[0]; viewportY=rectangle[1]; viewportWidth=rectangle[2]; viewportHeight=rectangle[3];
    gl.viewport(viewportX,displayHeight-viewportY-viewportHeight,viewportWidth,viewportHeight);
    viewportScale=static_cast<float>(viewportWidth)*0.0015625f;
    pixelScale=(static_cast<float>(viewportWidth)*nearPlane)/(right-left);
}
}
