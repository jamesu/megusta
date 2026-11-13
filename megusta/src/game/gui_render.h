// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "gui_assets.h"
namespace megusta {
class OpenGLAdapter;
struct GuiAtlasRegistry {
    GuiTextureAtlas* atlases[15]{};
    GuiTextureAtlas *generalAtlas=nullptr,*shellGeneralAtlas=nullptr;
    bool shellOpen=false;
    ResourceMountTable& resources;
    explicit GuiAtlasRegistry(ResourceMountTable& resources):resources(resources) {}
    std::uint32_t resolve(int atlas,int texture);
};
// Typed view of the original renderer globals, not a recovered class layout.
class GuiRenderer {
public:
    GuiRenderer(OpenGLAdapter& gl,GuiAtlasRegistry& atlases,GuiWidgetResources& widgets):gl(gl),atlases(atlases),widgets(widgets) {}
    OpenGLAdapter& gl;
    GuiAtlasRegistry& atlases;
    GuiWidgetResources& widgets;
    float gradientBottom=0,gradientTop=0;
    const float *gradientTopColor=nullptr,*gradientBottomColor=nullptr;
    void deleteTexture(std::uint32_t);
    void beginTexturedAlpha(float); void endTexturedAlpha();
    void drawTextureRect(std::uint32_t,int x,int top,int width,int height,bool manageState);
    void drawCornerColorRect(int left,int top,int right,int bottom,const float colors[4][4]);
    void drawNineSliceFrame(int left,int bottom,int right,int top,int style);
    void setVerticalGradientColor(int x,int y,float alpha);
    void drawGradientTextureRect(std::uint32_t,int x,int top,int width,int height,float alpha);
    void drawGradientShadowBox(int left,int bottom,int right,int top,const float* topColor,const float* bottomColor,float alpha);
};
}
