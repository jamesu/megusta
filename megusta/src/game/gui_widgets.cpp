#include "gui_widgets.h"
#include "driver.h"
#include "gui_runtime.h"
#include "gui_render.h"
#include "scene_runtime.h"
#include "engine/opengl_adapter.h"
#include "engine/runtime_adapters.h"
#include "engine/string_services.h"
#include "ssg.h"
#include <cmath>
#include <cstdio>
#include <cstring>
namespace megusta {
namespace { GuiSceneButton* pendingPreview=nullptr; }
GuiFrame::GuiFrame(int x,int y,int right,int top):puObject(x,y,right,top) { type|=0x80000010u; }
void GuiFrame::draw(int dx,int dy) {
    if (visible && window==puGetWindow()) guiRuntime->renderer->drawNineSliceFrame(
        abox.min[0]+dx,abox.min[1]+dy,abox.max[0]+dx,abox.max[1]+dy,0);
}
GuiShadowBox::GuiShadowBox(int x,int y,int right,int top,int milliseconds):puObject(x,y,right,top) {
    type|=0x80000000u; sort_priority=-1; fade=1;
    if (milliseconds<1) { fade=0; fadeRate=0; } else fadeRate=1000.0f/milliseconds;
    setCornerColor(0,0.5f,0.5f,0.5f,0.4f); setCornerColor(1,0.5f,0.5f,0.5f,0.4f);
    setCornerColor(2,0.5f,0.5f,0.5f,0.2f); setCornerColor(3,0.5f,0.5f,0.5f,0.2f);
    setValue(1.0f);
}
void GuiShadowBox::setCornerColor(int index,float r,float g,float b,float a) {
    if (index>=0 && index<4) { auto* color=cornerColors[index]; color[0]=r; color[1]=g; color[2]=b; color[3]=a; }
}
void GuiShadowBox::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    if (parseLegend && legend && *legend) {
        parseLegend=false;
        std::sscanf(legend,"%f,%f,%f,%f, %f,%f,%f,%f, %f,%f,%f,%f, %f,%f,%f,%f",
            &cornerColors[0][0],&cornerColors[0][1],&cornerColors[0][2],&cornerColors[0][3],
            &cornerColors[1][0],&cornerColors[1][1],&cornerColors[1][2],&cornerColors[1][3],
            &cornerColors[2][0],&cornerColors[2][1],&cornerColors[2][2],&cornerColors[2][3],
            &cornerColors[3][0],&cornerColors[3][1],&cornerColors[3][2],&cornerColors[3][3]);
    }
    fade-=guiRuntime->system.frameDelta*fadeRate;
    float alpha=getFloatValue(); if (fade>0) alpha=(1.0f-fade)*alpha;
    guiRuntime->renderer->drawGradientShadowBox(abox.min[0]+dx,abox.min[1]+dy,abox.max[0]+dx,
        abox.max[1]+dy,cornerColors[0],cornerColors[2],alpha);
}
GuiSimpleButton::GuiSimpleButton(int x,int y,int right,int top):puObject(x,y,right,top) {
    type|=0x800000c0u;
    for (int i=0;i<5;++i) {
        highlightPalette[i][0]=colour[i][0]; highlightPalette[i][1]=colour[i][2];
        highlightPalette[i][2]=colour[i][1]; highlightPalette[i][3]=colour[i][3];
    }
    auto& widgets=guiRuntime->renderer->widgets;
    if (!widgets.simpleButtonInitialized) widgets.markSimpleButtonInitialized();
}
void GuiSimpleButton::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto& runtime=*guiRuntime; auto& gl=runtime.renderer->gl;
    const bool hovered=isHit(runtime.input.mouse.x,runtime.screenHeight-runtime.input.mouse.y) &&
        runtime.system.isWidgetInputAllowed(this);
    int visual=0;
    if (!highlighted) { if (hovered) { visual=1; if (!visualState) playGuiAcceptSound(); } }
    else { visual=2; if (visualState!=2) playGuiDisabledSound(this); }
    visualState=visual; fade-=runtime.system.frameDelta*0.25f;
    auto* palette=visual==1?highlightPalette:colour;
    const int value=getValue();
    abox.draw(dx,dy,value!=highlighted?-style:style,palette,am_default);
    if (!active) gl.color4f(palette[4][0],palette[4][1],palette[4][2],palette[4][3]*0.5f);
    else gl.color4fv(palette[4]);
    const int textWidth=legendFont.getStringWidth(legend),textHeight=legendFont.getStringHeight();
    legendFont.drawString(legend,abox.min[0]+((abox.max[0]-abox.min[0])-textWidth)/2+dx,
        abox.min[1]+((abox.max[1]-abox.min[1])-textHeight)/2+dy);
    draw_label(dx,dy);
}
GuiStatusBar::GuiStatusBar(int x,int y,int right,int top,int image):puObject(x,y,right,top),textureIndex(image) { type|=0x80000000u; }
void GuiStatusBar::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto& runtime=*guiRuntime; auto& renderer=*runtime.renderer;
    float delta=runtime.system.frameDelta+runtime.system.frameDelta;
    const float target=getFloatValue();
    if (!(target<=displayValue)) {
        delta+=displayValue;
        if (target<=delta) delta=target; else if (delta<=0) delta=0;
        displayValue=delta;
    } else if (!(displayValue<=target)) {
        delta=displayValue-delta;
        if (delta<9999.0f) { if (delta<=target) delta=target; } else delta=9999.0f;
        displayValue=delta;
    }
    if (displayValue<1.0f) { if (displayValue<=0) displayValue=0; } else displayValue=1;
    const int left=abox.min[0]+dx,top=abox.max[1]+dy,bottom=abox.min[1]+dy;
    const int right=static_cast<int>((abox.max[0]-abox.min[0]+1)*static_cast<long double>(displayValue)+left);
    if (textureIndex>=0) {
        renderer.beginTexturedAlpha(1);
        renderer.drawTextureRect(renderer.atlases.resolve(0,textureIndex),left,top,8,16,false);
        renderer.drawTextureRect(renderer.atlases.resolve(0,textureIndex+1),left+8,top,right-left-15,16,false);
        renderer.drawTextureRect(renderer.atlases.resolve(0,textureIndex+2),right-7,top,8,16,false);
        renderer.endTexturedAlpha();
    } else {
        // The saved FSUBR at 00405854 computes 1.0f - the clamped
        // displayValue (the decompiler exposed the x87 ST(1) alias).
        runtime.statusFillColors[1][1]=1-displayValue; runtime.statusFillColors[2][1]=1-displayValue;
        renderer.drawCornerColorRect(left+3,top-2,right+3,bottom-2,runtime.statusShadowColors);
        renderer.drawCornerColorRect(left,top,right,bottom,runtime.statusFillColors);
    }
}
GuiBitmapButton::GuiBitmapButton(int x,int y):puObject(x,y,x,y) { type|=0x80000000u; }
GuiBitmapButton::~GuiBitmapButton() {
    if (bitmap && ownsBitmap) guiRuntime->renderer->deleteTexture(bitmap);
    bitmap=0; ownsBitmap=false;
}
void GuiBitmapButton::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto* owner=static_cast<guiElement*>(getUserData()); if (!owner) return;
    const char *image=owner->getAttr(4),*labelValue=owner->getAttr(2);
    auto& runtime=*guiRuntime; auto& renderer=*runtime.renderer;
    if (image && *image && (!labelValue || !*labelValue || std::strcmp(image,labelValue))) {
        if (bitmap && ownsBitmap) renderer.deleteTexture(bitmap);
        bitmap=0; ownsBitmap=false; owner->setAttr(2,image);
        if (*image=='#') {
            int index=0; width=64; height=64;
            std::sscanf(image+1,"%d,%d,%d,%f,%f",&index,&width,&height,&fade,&fadeRate);
            bitmap=renderer.atlases.resolve(0,index);
        } else { runtime.loadPng(runtime.context,bitmap,image,width,height); ownsBitmap=true; }
    }
    if (bitmap) {
        float alpha=1; if (fade>0) alpha=1-fade;
        fade-=runtime.system.frameDelta*fadeRate;
        if (getFloatValue()>=0) {
            renderer.beginTexturedAlpha(alpha);
            renderer.drawTextureRect(bitmap,abox.min[0]+dx,abox.max[1]+dy,width,height,false);
            renderer.endTexturedAlpha();
        }
    }
}
GuiAnimatedBitmapButton::GuiAnimatedBitmapButton(int x,int y,int right,int top):puObject(x,y,right,top) { type|=0x80000040u; }
void GuiAnimatedBitmapButton::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto* owner=static_cast<guiElement*>(getUserData()); if (!owner) return;
    const char *image=owner->getAttr(4),*labelValue=owner->getAttr(2);
    auto& runtime=*guiRuntime; auto& renderer=*runtime.renderer;
    if (image && *image && (!labelValue || !*labelValue || std::strcmp(image,labelValue))) {
        owner->setAttr(2,image); if (*image!='#') engine_runtime::engineRuntime_fatalExit_impl();
        width=height=offsetX=offsetY=atlasId=0; frameRate=10;
        std::memset(stateFrameOffsets,0,sizeof(stateFrameOffsets));
        std::memset(stateFrameCounts,0,sizeof(stateFrameCounts));
        int firstImage=0;
        std::sscanf(image+1,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&atlasId,&firstImage,
            stateFrameCounts,stateFrameCounts+1,stateFrameCounts+2,stateFrameCounts+3,stateFrameCounts+4,
            &offsetX,&offsetY,&width,&height,&frameRate);
        stateFrameOffsets[0]=firstImage;
        for (int i=1;i<5;++i) stateFrameOffsets[i]=stateFrameOffsets[i-1]+stateFrameCounts[i-1];
    }
    float alpha=1; if (fade>0) alpha=1-fade;
    fade-=runtime.system.frameDelta+runtime.system.frameDelta; elapsedSeconds+=runtime.system.frameDelta;
    const bool hovered=isHit(runtime.input.mouse.x,runtime.screenHeight-runtime.input.mouse.y) && runtime.system.isWidgetInputAllowed(this);
    int visual=0;
    if (!active) visual=3;
    else if (!highlighted) { if (hovered) { visual=1; if (!visualState) playGuiAcceptSound(); } }
    else { visual=2; if (visualState!=2) playGuiDisabledSound(this); }
    int frameState=visual;
    if (!stateFrameCounts[visual]) {
        if (visual==2 && stateFrameCounts[1]) frameState=1;
        else { if (visual==3) alpha*=0.3f; frameState=0; }
    }
    if (!stateFrameCounts[frameState]) engine_runtime::engineRuntime_fatalExit_impl();
    const int elapsedFrames=static_cast<int>(static_cast<long double>(frameRate)*elapsedSeconds);
    const int imageIndex=elapsedFrames%stateFrameCounts[frameState]+stateFrameOffsets[frameState];
    renderer.beginTexturedAlpha(alpha);
    renderer.drawTextureRect(renderer.atlases.resolve(atlasId,imageIndex),offsetX+abox.min[0]+dx,
        offsetY+abox.max[1]+dy,width,height,false);
    renderer.endTexturedAlpha(); visualState=visual;
}
GuiSceneButton::GuiSceneButton():puObject(-10,-10,-10,-10),sceneRoot(new ssgRoot) {
    type|=0x80000000u; pendingPreview=nullptr;
}
GuiSceneButton::~GuiSceneButton() {
    if (sceneRoot) { sceneRoot->removeAllKids(); delete sceneRoot; sceneRoot=nullptr; }
    pendingPreview=nullptr;
}
void GuiSceneButton::draw(int,int) {
    if (visible && window==puGetWindow()) {
        if (pendingPreview) engine_runtime::engineRuntime_fatalExit_impl();
        pendingPreview=this;
    }
}
void GuiSceneButton::addSceneChild(ssgEntity* child) { sceneRoot->addKid(child); }
int GuiSceneButton::specialCommand(const char* command) {
    if (!strings::compareInsensitive(command,"TEST")) {
        auto* entity=guiRuntime->loadScene(guiRuntime->context,"GOLFBA9",true);
        previewEntity=createSceneTransform(0,0,0,0); previewEntity->addKid(entity); addSceneChild(previewEntity);
    }
    return 0;
}
void GuiSceneButton::renderPreview() {
    auto& runtime=*guiRuntime; auto& gl=runtime.renderer->gl;
    runtime.previewAngle+=runtime.system.frameDelta*10.0f;
    sgVec3 axis={1,1,1}; sgMat4 rotation,translation,savedView;
    sgMakeRotMat4(rotation,runtime.previewAngle,axis);
    sgMakeTransMat4(translation,runtime.previewTranslation); sgPostMultMat4(rotation,translation);
    previewEntity->setTransform(rotation);
    auto* context=ssgGetCurrentContext(); float width,height;
    context->getFOV(&width,&height); context->getModelviewMatrix(savedView);
    sgCoord origin={{0,0,0},{0,0,0}}; context->setCamera(&origin); context->setFOV(45.000103f,0);
    gl.clearFlags(1);
    if (scissorEnabled) {
        const auto clip=driverViewport.pixelScissor(scissor[0],scissor[1],scissor[2],scissor[3]);
        gl.enable(0x66); gl.scissor(clip.x,clip.y,clip.width,clip.height);
    }
    gl.enable(0x21); ssgCullAndDraw(sceneRoot); gl.disable(0x21);
    if (scissorEnabled) gl.disable(0x66);
    context->setFOV(width,height); context->setModelviewMatrix(savedView);
}
void flushGuiScenePreview() {
    if (pendingPreview) { pendingPreview->renderPreview(); pendingPreview=nullptr; }
}
}
