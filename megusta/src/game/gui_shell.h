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
struct GuiShellActions {
    void* context;
    int (*playMovie)(void*,const char*,int,int,int,int,int);
    void (*configureAutorun)(void*,int,int,int);
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
