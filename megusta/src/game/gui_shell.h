// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "gui_assets.h"
namespace megusta {
class guiSystem;
struct GuiShellState {
    GuiTextureAtlas* backgroundAtlas=nullptr;
    float slideRemaining=0,animationTime=0;
    fntTexFont* fonts[10]{};
    GuiTextureAtlas* generalAtlas=nullptr;
};
struct GuiFontRegistry {
    fntTexFont* slots[10]{};
    fntTexFont *primary=nullptr,*console=nullptr,*secondary=nullptr;
};
extern GuiShellState* guiShellState;
extern GuiFontRegistry guiFonts;
void openGuiShell();
void closeGuiShell();
bool isGuiShellOpen();
void releaseGuiSharedAssets();
void destroyGuiTextures(GuiTextureAtlas*);
fntTexFont* resolveGuiFont(int);
}
