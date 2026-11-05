#include "gui_widgets.h"
#include "gui_runtime.h"
#include "gui_render.h"
#include "engine/opengl_adapter.h"
#include "engine/string_services.h"
#include <cstdio>
#include <cstdlib>
namespace megusta {
namespace {
float staggerLozengeFade(int kind) {
    auto& runtime=*guiRuntime;
    const float now=static_cast<std::int32_t>(runtime.milliseconds(runtime.context))*0.001f;
    auto& fade=runtime.lozengeStaggerFade[kind];
    if (runtime.lozengePreviousTime[kind]+0.1<=now) fade=0.8f;
    else { fade+=0.07; if (fade<1.6f) { if (fade<=0) fade=0; } else fade=1.6f; }
    runtime.lozengePreviousTime[kind]=now;
    return fade;
}
float clampedFade(float value) {
    if (value<1) { if (value<=0) value=0; } else value=1;
    return value;
}
void drawLozengeLegend(OpenGLAdapter& gl,puFont font,const char* text,int x,int y,
    int visual,bool active,float fade,bool dual) {
    float alpha=1;
    if (fade>0.5f) alpha=0;
    else if (fade>0) alpha=1-(fade+fade);
    const auto pass=[&](float r,float g,float b,float a,int px,int py) {
        gl.color4f(r,g,b,a); font.drawString(text,px,py);
    };
    if (!active) { pass(0.5f,0.5f,0.5f,alpha*0.5f,x,y); return; }
    if (visual==2) {
        pass(0,0,0,alpha*0.15f,x-1,y-1); pass(0,0,0,alpha*0.5f,x+2,y-4);
        pass(0,0,0,alpha*0.6f,x+1,y-3); pass(0.5f,0.5f,0.5f,alpha,x,y-2);
    } else {
        pass(0,0,0,alpha*0.3f,x-1,y+1); pass(0,0,0,alpha*0.7f,x+2,y-2);
        pass(0,0,0,alpha,x+1,y-1);
        if (visual==1) pass(1,1,1,alpha,x,y);
        else if (dual) pass(0.95f,0.71f,0.05f,alpha,x,y);
        else pass(0.76f,0.68f,0.26f,alpha,x,y);
    }
}
}
GuiToggleLozenge::GuiToggleLozenge(int x,int y,int right,int top,unsigned flags,int atlas,int image,int extent)
    :puObject(x,y,right,top),atlasId(atlas),firstImage(image),cellExtent(extent?extent:40),options(flags) {
    type|=0x80000040u;
    auto& renderer=*guiRuntime->renderer;
    fontSize=renderer.widgets.togglePointSize;
    fade=staggerLozengeFade(0);
    if (!renderer.widgets.toggleInitialized) renderer.widgets.loadToggleLozenge(guiAssetServices(renderer.atlases.resources));
}
int GuiToggleLozenge::specialCommand(const char* command,int font,int size,int baseline) {
    if (!strings::compareInsensitive(command,"setfont")) { fontId=font; fontSize=size; baselineOffset=baseline; }
    return 0;
}
void GuiToggleLozenge::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto& runtime=*guiRuntime; auto& renderer=*runtime.renderer;
    fade-=runtime.system.frameDelta+runtime.system.frameDelta;
    const float phase=clampedFade(fade);
    const bool hovered=isHit(runtime.input.mouse.x,runtime.screenHeight-runtime.input.mouse.y) && runtime.system.isWidgetInputAllowed(this);
    int state=0;
    if (!highlighted) { if (hovered) { state=1; if (!visualState) playGuiAcceptSound(); } }
    else { state=2; if (visualState!=2) playGuiDisabledSound(this); }
    int imageState=state;
    if (state!=1 && getValue()==1 && !(options&1)) imageState=state?0:2;
    const int height=abox.max[1]-abox.min[1]+1;
    int imageHeight=height*64/cellExtent;
    const int capWidth=height*32/cellExtent,left=abox.min[0]-5+dx;
    const int width=abox.max[0]-abox.min[0]+11,top=height*5/cellExtent+abox.max[1]+2+dy;
    float alpha=active?1.0f:0.5f;
    if (phase>0.5f) {
        const long double factor=1.0L-((static_cast<long double>(phase)-0.5L)+(static_cast<long double>(phase)-0.5L));
        imageHeight=(static_cast<int>(height*factor)*64)/cellExtent; alpha=static_cast<float>(factor*alpha);
    }
    const auto image=[&](int part) {
        int index=imageState*3+part;
        return !firstImage?pix::getMultiTexture(renderer.widgets.toggleAtlas,index):renderer.atlases.resolve(atlasId,firstImage+index);
    };
    renderer.beginTexturedAlpha(alpha);
    renderer.drawTextureRect(image(0),left,top,capWidth,imageHeight,false);
    if (width>64) renderer.drawTextureRect(image(1),left+capWidth,top,width-2*capWidth,imageHeight,false);
    renderer.drawTextureRect(image(2),left+width-capWidth,top,capWidth,imageHeight,false); renderer.endTexturedAlpha();
    puFont font(fontId>=0?runtime.resolveFont(runtime.context,fontId):renderer.widgets.toggleFont,fontSize,0);
    const int textX=abox.min[0]+(abox.max[0]-abox.min[0]-font.getStringWidth(legend))/2+dx;
    const int textY=abox.min[1]+(abox.max[1]-abox.min[1]-font.getStringHeight())/2+baselineOffset+dy;
    drawLozengeLegend(renderer.gl,font,legend,textX,textY,imageState,active,phase,false);
    draw_label(dx+10,dy); visualState=state;
}
GuiLozenge2::GuiLozenge2(int x,int y,int right,int top,unsigned flags):puObject(x,y,right,top),options(flags) {
    type|=0x80000040u;
    auto& renderer=*guiRuntime->renderer;
    fontSize=renderer.widgets.togglePointSize; fade=staggerLozengeFade(1);
    if (!renderer.widgets.lozengeInitialized) renderer.widgets.loadLozenge(guiAssetServices(renderer.atlases.resources));
}
bool GuiLozenge2::handleDirection(int horizontal,int vertical) {
    if (!vertical) {
        if (horizontal<0 && (options&2)) { dispatchGuiEvent(this,0); previousDirection=1; return true; }
        if (horizontal>0 && (options&4)) { dispatchGuiEvent(this,1); previousDirection=3; return true; }
    }
    return false;
}
void GuiLozenge2::activateCallback() {
    if (currentDirection==1) { dispatchGuiEvent(this,0); previousDirection=1; }
    else { dispatchGuiEvent(this,1); previousDirection=3; }
}
void guiLozengeActivateCallback(puObject* widget) { static_cast<GuiLozenge2*>(widget)->activateCallback(); }
int GuiLozenge2::specialCommand(const char* command,unsigned font,int size,int baseline) {
    if (!strings::compareInsensitive(command,"setflags")) options=font;
    if (!strings::compareInsensitive(command,"setfont")) { fontId=static_cast<int>(font); fontSize=size; baselineOffset=baseline; }
    return 0;
}
void GuiLozenge2::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    auto& runtime=*guiRuntime; auto& renderer=*runtime.renderer;
    fade-=runtime.system.frameDelta+runtime.system.frameDelta;
    const float phase=clampedFade(fade);
    const int mouseX=runtime.input.mouse.x;
    const bool hovered=isHit(mouseX,runtime.screenHeight-runtime.input.mouse.y) && runtime.system.isWidgetInputAllowed(this);
    currentDirection=0;
    if (hovered) {
        currentDirection=2;
        if (mouseX<abox.min[0]+64+dx) currentDirection=1;
        if (mouseX>abox.max[0]+dx-64) currentDirection=3;
    }
    int state=0;
    if (!highlighted) { if (hovered) { state=1; if (!visualState) playGuiAcceptSound(); } }
    else { state=2; if (visualState!=2) playGuiDisabledSound(this); }
    int textState=state;
    if (state!=1 && getValue()==1 && !(options&1)) textState=state?0:2;
    const int height=abox.max[1]-abox.min[1]+1,capWidth=height*64/40;
    int imageHeight=capWidth;
    const int left=abox.min[0]-5+dx,width=abox.max[0]-abox.min[0]+11,top=abox.max[1]+2+dy;
    float alpha=active?1.0f:0.5f;
    if (phase>0.5f) {
        const long double factor=1.0L-((static_cast<long double>(phase)-0.5L)+(static_cast<long double>(phase)-0.5L));
        imageHeight=(static_cast<int>(height*factor)*64)/40; alpha=static_cast<float>(factor*alpha);
    }
    int leftState=1,rightState=1;
    if (currentDirection==1) leftState=2;
    if (currentDirection==3) rightState=2;
    if (previousDirection==1) leftState=3;
    if (previousDirection==3) rightState=3;
    if (!(options&2)) leftState=0;
    if (!(options&4)) rightState=0;
    renderer.beginTexturedAlpha(alpha);
    const auto image=[&](int index) { return pix::getMultiTexture(renderer.widgets.lozengeAtlas,index); };
    renderer.drawTextureRect(image(leftState+1),left,top,capWidth,imageHeight,false);
    if (width>128) renderer.drawTextureRect(image(0),left+capWidth,top,width-2*capWidth,imageHeight,false);
    renderer.drawTextureRect(image(rightState+5),left+width-capWidth,top,capWidth,imageHeight,false);
    renderer.endTexturedAlpha(); previousDirection=0;
    puFont font(fontId>=0?runtime.resolveFont(runtime.context,fontId):renderer.widgets.toggleFont,fontSize,0);
    const int textX=abox.min[0]+(abox.max[0]-abox.min[0]-font.getStringWidth(legend))/2+dx;
    const int textY=abox.min[1]+(abox.max[1]-abox.min[1]-font.getStringHeight())/2+baselineOffset+dy;
    drawLozengeLegend(renderer.gl,font,legend,textX,textY,textState,active,phase,true);
    draw_label(dx+10,dy); visualState=state;
}
GuiHeader::GuiHeader(int x,int y,int right,int top,bool animate):puObject(x,y,right,top) {
    type|=0x80000000u; auto& renderer=*guiRuntime->renderer;
    animationPhase=animate && renderer.atlases.shellOpen?0.61f:0.0f;
    if (!renderer.widgets.headerInitialized) renderer.widgets.loadHeaderFont();
}
void GuiHeader::draw(int,int) {
    if (!visible || window!=puGetWindow()) return;
    auto& renderer=*guiRuntime->renderer; auto& gl=renderer.gl;
    animationPhase-=guiRuntime->system.frameDelta*0.7;
    float alpha=1.0333333f-animationPhase*8;
    if (!(alpha>=0)) return;
    if (alpha>1) alpha=1;
    int x=120; float slant;
    if (animationPhase<=0) slant=animationPhase<-0.033333335f?0.0f:animationPhase*1140.0f+33.0f;
    else { x=static_cast<int>(static_cast<long double>(animationPhase)*1500+120); slant=33; }
    puFont font(renderer.widgets.headerFont,renderer.widgets.headerPointSize,slant);
    const float weights[6]={0.025f,0.075f,0.1f,0.15f,0.2f,0.25f};
    for (int i=0;i<6;++i) { gl.color4f(0,0,0,alpha*weights[i]); font.drawString(label,x+6-i,409+i); }
    gl.color4f(0.95703125f,0.9765625f,0.0390625f,alpha); font.drawString(label,x,415);
}
GuiText::GuiText(int x,int y):puObject(x,y,x,y) {
    type|=0x80000020u;
    auto& widgets=guiRuntime->renderer->widgets;
    if (!widgets.textInitialized) widgets.markTextInitialized();
}
int GuiText::specialCommand(const char* command) {
    if (!strings::compareInsensitive(command,"nofade")) fade=-1;
    return 0;
}
void GuiText::draw(int dx,int dy) {
    if (!visible || window!=puGetWindow()) return;
    if (!parsed) {
        parsed=true; fontId=203; fontSize=18; alignmentScale=0; drawStyle=2;
        int colors[8]={255,255,255,255,0,0,0,128},disableFade=0;
        if (!legend || !*legend) { constexpr int alignmentValueUnit=256; alignmentScale=getValue()/alignmentValueUnit-1; }
        else {
            if (*legend!='#') std::abort();
            std::sscanf(legend+1,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&fontId,&fontSize,&alignmentScale,&drawStyle,
                colors,colors+1,colors+2,colors+3,colors+4,colors+5,colors+6,colors+7,&disableFade);
        }
        for (int i=0;i<4;++i) { primaryColor[i]=colors[i]*0.003921569f; secondaryColor[i]=colors[i+4]*0.003921569f; }
        if (disableFade) fade=0;
    }
    fade=clampedFade(fade-(guiRuntime->system.frameDelta+guiRuntime->system.frameDelta));
    float alpha=1-fade; if (!active) alpha*=0.5f;
    puFont font(guiRuntime->resolveFont(guiRuntime->context,fontId),fontSize,0);
    const int y=abox.min[1]+font.getStringDescender()+dy;
    const int x=abox.min[0]+dx-(font.getStringWidth(label)*(alignmentScale+1))/2;
    auto& gl=guiRuntime->renderer->gl;
    if (drawStyle<0 || drawStyle>3) std::abort();
    if (drawStyle) {
        gl.color4f(secondaryColor[0],secondaryColor[1],secondaryColor[2],alpha*secondaryColor[3]);
        font.drawString(label,x+drawStyle,y-drawStyle);
    }
    gl.color4f(primaryColor[0],primaryColor[1],primaryColor[2],alpha*primaryColor[3]); font.drawString(label,x,y);
}
}
