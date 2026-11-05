#include "gui_render.h"
#include "engine/opengl_adapter.h"
#include "engine/runtime_adapters.h"
namespace megusta {
std::uint32_t GuiAtlasRegistry::resolve(int atlas,int texture) { // 0040ae80
    if (atlas>=0 && atlas<15) return pix::getMultiTexture(atlases[atlas],texture);
    if (atlas==100) {
        if (shellOpen) return pix::getMultiTexture(shellGeneralAtlas,texture);
        if (!generalAtlas) {
            generalAtlas=pix::loadMultiTexture(resources,"data/textures/gui/GENER.mpx",true,0,0x2900,0x2600,0x2600);
            if (!generalAtlas) engine_runtime::engineRuntime_fatalExit_impl();
        }
        return pix::getMultiTexture(generalAtlas,texture);
    }
    engine_runtime::engineRuntime_fatalExit_impl();
}
void GuiRenderer::deleteTexture(std::uint32_t name) { gl.deleteTextures(1,&name); }
void GuiRenderer::beginTexturedAlpha(float alpha) { gl.enable(0x4a); gl.enable(0x2e); gl.blendFunc(0x37,0x38); gl.color4f(1,1,1,alpha); }
void GuiRenderer::endTexturedAlpha() { gl.disable(0x4a); gl.disable(0x2e); gl.blendFunc(0x32,0x31); gl.color4f(1,1,1,1); }
void GuiRenderer::drawTextureRect(std::uint32_t texture,int x,int top,int width,int height,bool manageState) {
    if (width<=0 || height<=0) return;
    if (manageState) beginTexturedAlpha(1);
    gl.bindTexture(0x4a,texture); gl.begin(0xb);
    gl.texCoord2f(0,0); gl.vertex3f(x,top,0);
    gl.texCoord2f(1,0); gl.vertex3f(x+width,top,0);
    gl.texCoord2f(1,1); gl.vertex3f(x+width,top-height,0);
    gl.texCoord2f(0,1); gl.vertex3f(x,top-height,0); gl.end();
    if (manageState) endTexturedAlpha();
}
void GuiRenderer::drawCornerColorRect(int left,int top,int right,int bottom,const float colors[4][4]) {
    gl.disable(0x4a); gl.enable(0x2e); gl.blendFunc(0x37,0x38); gl.shadeModel(0x3d); gl.begin(0xb);
    gl.color4fv(colors[0]); gl.vertex3f(left,top,0);
    gl.color4fv(colors[1]); gl.vertex3f(right,top,0);
    gl.color4fv(colors[2]); gl.vertex3f(right,bottom,0);
    gl.color4fv(colors[3]); gl.vertex3f(left,bottom,0); gl.end(); endTexturedAlpha();
}
void GuiRenderer::drawNineSliceFrame(int left,int bottom,int right,int top,int style) {
    int x=left-4,y=top+4,width=(right+18)-x,height=y-(bottom-24),base=style*9;
    beginTexturedAlpha(1);
    auto draw=[&](int index,int x,int y,int width,int height) {
        drawTextureRect(atlases.resolve(100,base+index),x,y,width,height,false);
    };
    draw(44,left+16,top-16,width-40,height-60);
    draw(41,left+60,y,width-128,64);
    draw(43,x,top-60,64,height-128);
    draw(45,right-46,top-60,64,height-128);
    draw(47,left+60,bottom+40,width-128,64);
    draw(40,x,y,64,64); draw(42,right-46,y,64,64);
    draw(46,x,bottom+40,64,64); draw(48,right-46,bottom+40,64,64);
    endTexturedAlpha();
}
void GuiRenderer::setVerticalGradientColor(int,int y,float alpha) {
    float topWeight=(static_cast<float>(y)-gradientBottom)/(gradientTop-gradientBottom);
    if (topWeight<1) { if (topWeight<=0) topWeight=0; } else topWeight=1;
    float bottomWeight=1-topWeight;
    gl.color4f(topWeight*gradientTopColor[0]+bottomWeight*gradientBottomColor[0],
        topWeight*gradientTopColor[1]+bottomWeight*gradientBottomColor[1],
        topWeight*gradientTopColor[2]+bottomWeight*gradientBottomColor[2],
        (topWeight*gradientTopColor[3]+bottomWeight*gradientBottomColor[3])*alpha);
}
void GuiRenderer::drawGradientTextureRect(std::uint32_t texture,int x,int top,int width,int height,float alpha) {
    if (width<=0 || height<=0) return;
    gl.bindTexture(0x4a,texture); gl.begin(0xb);
    setVerticalGradientColor(x,top,alpha); gl.texCoord2f(0,0); gl.vertex3f(x,top,0);
    setVerticalGradientColor(x+width,top,alpha); gl.texCoord2f(1,0); gl.vertex3f(x+width,top,0);
    setVerticalGradientColor(x+width,top-height,alpha); gl.texCoord2f(1,1); gl.vertex3f(x+width,top-height,0);
    setVerticalGradientColor(x,top-height,alpha); gl.texCoord2f(0,1); gl.vertex3f(x,top-height,0); gl.end();
}
void GuiRenderer::drawGradientShadowBox(int left,int bottom,int right,int top,const float* topColor,const float* bottomColor,float alpha) {
    beginTexturedAlpha(1);
    gradientBottom=bottom; gradientTop=top; gradientTopColor=topColor; gradientBottomColor=bottomColor;
    int width=right-left-64,height=top-bottom-64;
    auto draw=[&](int index,int x,int y,int w,int h) { drawGradientTextureRect(widgets.shadowAtlas->textureNames[index],x,y,w,h,alpha); };
    draw(4,left+32,top-32,width,height); draw(1,left+32,top,width,32);
    draw(3,left,top-32,32,height); draw(5,right-32,top-32,32,height);
    draw(7,left+32,bottom+32,width,32); draw(0,left,top,32,32);
    draw(2,right-32,top,32,32); draw(6,left,bottom+32,32,32); draw(8,right-32,bottom+32,32,32);
    endTexturedAlpha();
}
}
