// SPDX-License-Identifier: GPL-2.0

#pragma once
namespace megusta {
class OpenGLAdapter;
void loadConvertedMatrix(OpenGLAdapter&,float*);
void multiplyConvertedMatrix(OpenGLAdapter&,float*);
struct RenderProjectionState {
    float left,right,bottom,top,nearPlane,farPlane,pixelScale,viewportScale;
    int viewportX,viewportY,viewportWidth,viewportHeight;
    bool orthographic;
    float projectionAdjustment[16];
    void setProjectionBounds(OpenGLAdapter&,float,float,float,float,float,float,bool);
    void setViewport(OpenGLAdapter&,const int* rectangle,int displayHeight);
};
}
