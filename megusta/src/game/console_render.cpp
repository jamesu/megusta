#include "console.h"
#include "../engine/opengl_adapter.h"
#include "fnt.h"
#include <cmath>
#include <cstring>
namespace megusta {
extern ConsoleServices* consoleServices;
namespace {
constexpr char ConsoleScrollMarker[]="*"; // Exact bytes at 004f2b20.
}
void Console::drawTextAtCell(int row,int column,const char* text) {
    auto& services=*consoleServices;
    float x=(static_cast<float>(column)+1.0)*(0.9/static_cast<float>(services.columns))*static_cast<float>(services.displayWidth);
    float y=((0.4/static_cast<float>(services.rows))*static_cast<float>(row)+0.08)*static_cast<float>(services.displayHeight);
    renderer->start3f(x,y,0); renderer->puts(text);
}
void Console::drawOverlay() {
    if (!visible) return;
    auto& services=*consoleServices; auto& gl=services.gl;
    gl.shadeModel(0x3d); gl.blendFunc(0x37,0x38); gl.enable(0x2e);
    gl.matrixMode(0xf); glLoadIdentity(); gl.ortho(0,1,0,1,-1,1);
    gl.matrixMode(0xe); glLoadIdentity(); gl.depthFunc(0x2d); gl.disable(0x4a);
    gl.begin(0xb); gl.color4f(0.2f,0.2f,0.6f,0.3f);
    gl.vertex3f(0,0.52f,-0.5f); gl.vertex3f(0,0.05f,-0.5f); gl.vertex3f(1,0.05f,-0.5f); gl.vertex3f(1,0.52f,-0.5f);
    gl.color4f(0,0,0,0.6f);
    gl.vertex3f(0,0.51f,-0.5f); gl.vertex3f(0,0.06f,-0.5f); gl.vertex3f(1,0.06f,-0.5f); gl.vertex3f(1,0.51f,-0.5f); glEnd();
    int width=services.queryWindowDimension(1),height=services.queryWindowDimension(2);
    gl.matrixMode(0xf); glLoadIdentity(); gl.ortho(0,static_cast<float>(width),0,static_cast<float>(height),0,1);
    gl.matrixMode(0xe); glLoadIdentity(); renderer->getFont()->begin();
    int index=outputViewIndex;
    for (int row=0;row<services.rows;++row) {
        for (auto& component:outputColors[outputWriteIndex]) component=1;
        gl.color4fv(outputColors[index]); drawTextAtCell(row,0,outputLines[index]); index=(index+1)%OutputLineCount;
    }
    if (outputViewIndex!=outputWriteIndex) drawTextAtCell(0,-1,ConsoleScrollMarker);
    gl.color4f(0,1,0,0.5f);
    drawTextAtCell(services.rows-1,services.columns-static_cast<int>(std::strlen(title)),title);
    renderer->getFont()->end();
    if (opening) {
        auto elapsed=services.timeMilliseconds()-toggleTime;
        if (elapsed>249) opening=0;
        gl.disable(0x4a); gl.blendFunc(0x32,0x32); gl.begin(0xb);
        float angle=static_cast<float>(static_cast<long double>(elapsed)*0.0062831853072L);
        float dim=static_cast<float>(std::cos(static_cast<long double>(angle))*0.25L);
        float bright=static_cast<float>(std::cos(static_cast<long double>(angle))*0.5L);
        gl.color3f(bright,bright,bright); gl.vertex3f(0,0.51f,-0.5f); gl.vertex3f(1,0.51f,-0.5f);
        gl.color3f(dim,dim,dim); gl.vertex3f(1,0.57f,-0.5f); gl.vertex3f(0,0.57f,-0.5f);
        gl.vertex3f(0,0,-0.5f); gl.vertex3f(1,0,-0.5f);
        gl.color3f(bright,bright,bright); gl.vertex3f(1,0.06f,-0.5f); gl.vertex3f(0,0.06f,-0.5f);
        gl.vertex3f(0,0.51f,-0.5f); gl.vertex3f(0,0.06f,-0.5f); gl.vertex3f(1,0.06f,-0.5f); gl.vertex3f(1,0.51f,-0.5f);
        glEnd(); gl.blendFunc(0x37,0x38);
    }
    gl.depthFunc(0x29);
}
}
