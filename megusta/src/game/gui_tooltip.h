#pragma once
#include <cstdint>
#include "pu.h"
class fntTexFont;
namespace megusta {
class guiElement;
// Shared tooltip globals; gradient initial RGB data still requires recovery.
struct GuiTooltipState {
    guiElement* hoveredElement=nullptr;
    int hoveredRow=-1;
    bool fadingOut=false;
    const char* text=nullptr;
    std::uint32_t hoverStartMs=0;
    int visibleDurationMs=0,area=0,pointSize=20,lineHeight=22;
    float alpha=0,topColor[4]{},bottomColor[4]{};
    fntTexFont* font=nullptr;
    char* lines[33]{};
};
extern GuiTooltipState guiTooltipState;
void invalidateGuiTooltip(guiElement*);
void initializeGuiTooltip();
void resetGuiTooltipDefaults();
char** wrapGuiTooltipText(char*,puFont,int maximumWidth);
void updateGuiTooltipHover();
void drawGuiTooltip();
}
