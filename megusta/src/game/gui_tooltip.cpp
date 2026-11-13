#include "gui_tooltip.h"
#include "gui.h"
#include "gui_list.h"
#include "gui_runtime.h"
#include "gui_render.h"
#include "engine/opengl_adapter.h"
#include "engine/runtime_adapters.h"
#include "fnt.h"
#include "../engine/platform_gl.h"
#include <cstring>
namespace megusta {
GuiTooltipState guiTooltipState;
void invalidateGuiTooltip(guiElement* element) {
    if (element && element->attributes[5]==guiTooltipState.text) {
        guiTooltipState.hoveredElement=nullptr; guiTooltipState.hoveredRow=-1;
        guiTooltipState.fadingOut=false; guiTooltipState.text=nullptr;
    }
}
void resetGuiTooltipDefaults() {
    auto& system=guiRuntime->system;
    system.tooltipAreas[0][0]=300; system.tooltipAreas[0][1]=30;
    system.tooltipAreas[0][2]=320; system.tooltipAreas[0][3]=150;
    system.tooltipDelay=800;
    const int colors[4]={192,180,128,100};
    for (int i=0;i<4;++i) system.tooltipColor[i]=colors[i]*0.00390625f;
    guiTooltipState.hoveredElement=nullptr; guiTooltipState.hoveredRow=-1;
    guiTooltipState.fadingOut=false; guiTooltipState.text=nullptr;
}
void initializeGuiTooltip() {
    auto& state=guiTooltipState;
    if (!state.font) { state.font=new fntTexFont("data/fonts/tooltip.txf",GL_LINEAR,GL_LINEAR); state.pointSize=20; state.lineHeight=22; }
    resetGuiTooltipDefaults();
}
char** wrapGuiTooltipText(char* text,puFont font,int maximumWidth) {
    auto& lines=guiTooltipState.lines;
    std::memset(lines,0,sizeof(lines));
    if (!std::strchr(text,'\n') && font.getStringWidth(text)<maximumWidth) { lines[0]=text; return lines; }
    char* lastBreak=nullptr; char* cursor=text; char** output=lines;
    for (;;) {
        const char character=*cursor;
        bool advance=true;
        if (character<=' ' && character>=0) {
            *cursor='\0'; const int width=font.getStringWidth(text); *cursor=character;
            if (width<maximumWidth) lastBreak=cursor;
            if (width>maximumWidth || (character=='\0' && width<maximumWidth) || character=='\n') {
                *lastBreak='\0'; *output++=text; text=lastBreak+1; lastBreak=nullptr;
                while (*text==' ') ++text;
                cursor=text; advance=false;
            }
        }
        if (!*cursor) return lines;
        if (advance) ++cursor;
    }
}
void updateGuiTooltipHover() {
    auto& runtime=*guiRuntime; auto& system=runtime.system; auto& state=guiTooltipState;
    const int x=runtime.input.mouse.x,y=runtime.screenHeight-runtime.input.mouse.y;
    guiElement* hovered=nullptr;
    for (int i=0;i<system.elementCount;++i) {
        auto* element=system.elements[i]; const char* hint=element->attributes[5];
        if ((element->type==19 || (hint && *hint)) && element->x<=x && x<element->x+element->width &&
            element->y<=y && y<element->y+element->height && system.isWidgetInputAllowed(element->widget)) { hovered=element; break; }
    }
    const char* text=hovered?hovered->attributes[5]:nullptr;
    int row=-1;
    if (hovered && hovered->type==19) {
        auto* list=static_cast<GuiListButton*>(hovered->widget);
        if (list->hintColumn>0 && list->hoveredRow>=0) {
            char* item=list->getItemText(list->hoveredRow);
            for (int column=0;column<list->hintColumn;++column) {
                char* tab=std::strchr(item,'\t'); if (!tab) break; item=tab+1;
                if (column==list->hintColumn-1 && *item) {
                    if (!hovered->getAttr(4)) { char spaces[4097]; std::memset(spaces,' ',4096); spaces[4096]='\0'; hovered->setAttr(4,spaces); }
                    char* buffer=hovered->attributes[4]; *buffer='\0';
                    if (text) std::strcat(buffer,text); std::strcat(buffer,item); text=buffer;
                    row=list->hoveredRow; if (std::strlen(text)>4095) engine_runtime::engineRuntime_fatalExit_impl();
                }
            }
        }
    }
    if (hovered && (!text || !*text)) hovered=nullptr;
    const auto hide=[&]() { state.hoveredElement=nullptr; state.hoveredRow=-1; state.fadingOut=true; };
    if (!hovered || !text || runtime.input.buttons || runtime.input.keyCode) { hide(); return; }
    if (state.hoveredElement!=hovered || state.hoveredRow!=row) {
        if (state.hoveredElement) { hide(); return; }
        state.hoveredElement=hovered; state.hoveredRow=row;
        state.hoverStartMs=runtime.milliseconds(runtime.context); state.visibleDurationMs=10000;
    }
    const auto now=runtime.milliseconds(runtime.context);
    const int elapsed=static_cast<std::int32_t>(now)<static_cast<std::int32_t>(state.hoverStartMs)?
        static_cast<std::int32_t>(state.hoverStartMs-now):static_cast<std::int32_t>(now-state.hoverStartMs);
    if (elapsed<=state.visibleDurationMs+system.tooltipDelay) {
        if (elapsed<system.tooltipDelay || state.text) return;
        state.alpha=0.05f; state.fadingOut=false; state.area=0; state.text=text;
        if (text[0]=='#' && text[2]=='#') {
            state.area=text[1]-'0'; state.text=text+3;
            if (state.area<0 || state.area>3) engine_runtime::engineRuntime_fatalExit_impl();
        }
        state.visibleDurationMs=(static_cast<int>(std::strlen(state.text))*5+250)*20;
    } else hide();
}
void drawGuiTooltip() {
    auto& state=guiTooltipState; auto& runtime=*guiRuntime;
    if (!(state.alpha>0) || !state.text) return;
    if (!state.fadingOut) { state.alpha+=runtime.system.frameDelta+runtime.system.frameDelta; if (state.alpha>1) state.alpha=1; }
    else { state.alpha-=runtime.system.frameDelta*4; if (state.alpha<0) { state.text=nullptr; state.fadingOut=false; return; } }
    state.topColor[3]=runtime.system.tooltipColor[0]; state.bottomColor[3]=runtime.system.tooltipColor[2];
    const int* area=runtime.system.tooltipAreas[state.area];
    auto& renderer=*runtime.renderer;
    renderer.drawGradientShadowBox(area[0],area[1],area[0]+area[2],area[1]+area[3],state.topColor,state.bottomColor,state.alpha);
    if (state.font) {
        if (std::strlen(state.text)>1023) engine_runtime::engineRuntime_fatalExit_impl();
        char text[1024]{}; std::strcpy(text,state.text); puFont font(state.font,state.pointSize,0);
        char** lines=wrapGuiTooltipText(text,font,area[2]);
        int y=area[3]/2-state.lineHeight/2+area[1];
        for (auto** line=lines;*line;++line) y+=state.lineHeight/2;
        for (;*lines;++lines,y-=state.lineHeight) {
            const int x=area[2]/2+area[0]-font.getStringWidth(*lines)/2;
            renderer.gl.color4f(0,0,0,state.alpha*0.6f); font.drawString(*lines,x+1,y-1);
            renderer.gl.color4f(1,1,1,state.alpha); font.drawString(*lines,x,y);
        }
    }
}
}
