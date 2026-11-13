// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "gui.h"
#include "player.h"
class fntTexFont;
class ssgEntity;
namespace megusta {
class GuiRenderer;
class GuiVerticalSlider;
struct GuiShapeServices;
struct DriverScriptServices;
struct GuiNameCode { const char* name; std::uint32_t code; };
class iRoomClient;
struct GuiDebugState {
    bool drawViewportBorder=false;
    unsigned renderMode=0;
    bool& renderOptionLow;
    bool& renderOptionHigh;
};
void drawGuiChatOverlay(iRoomClient&,bool activeScene,const char* mapName,const GuiDebugState&);
struct GuiRuntime {
    guiSystem& system;
    GameInputFrame& input;
    void* context;
    void (*executePython)(void*,const char*);
    std::uint32_t (*milliseconds)(void*);
    std::uint32_t (*fixedTick)(void*);
    void (*mouse)(void*,int,int,int,int);
    bool (*soundPlaying)(void*,const char*,int);
    void (*playSound)(void*,const char*,int);
    const GuiNameCode *keyNames,*buttonNames;
    std::uint32_t previousFrameMs=0,buttonEdges=0;
    bool clickPending=false,messageActive=false;
    std::uint32_t clickStart=0,clickDuration=0,messageStart=0,messageDuration=0;
    char message[84]{};
    int screenWidth=0,screenHeight=0;
    void (*warpCursor)(void*,int,int);
    void (*enableCursorMotion)(void*);
    fntTexFont* (*resolveFont)(void*,int);
    GuiRenderer* renderer;
    bool (*loadPng)(void*,std::uint32_t&,const char*,int&,int&);
    ssgEntity* (*loadScene)(void*,const char*,bool);
    // Defaults recovered from the original .data globals at 004f2604-004f268c
    // and 00525aa4. A host may still replace them when binding its runtime.
    float statusFillColors[4][4]={{1,1,0,1},{1,0,0,1},{1,0,0,1},{1,1,1,0}};
    float statusShadowColors[4][4]={{0,0,0.5f,0},{0,0,0.5f,0},{0,0,0.5f,0},{0.8f,0,0,0.8f}};
    float previewTranslation[3]={-2,6,-1};
    float previewAngle=0;
    float lozengePreviousTime[2],lozengeStaggerFade[2];
    DriverScriptServices* scriptServices=nullptr;
    GuiShapeServices* shapes;
};
extern GuiRuntime* guiRuntime;
// Borrowed for the lifetime of GUI objects and their PUI callbacks.
void bindGuiRuntime(GuiRuntime&);
std::uint32_t lookupGuiNameCode(const GuiNameCode*,const char*);
void executeGuiBindingAction(const char*);
void repeatGameCallback(std::uint32_t,std::uint32_t,int,void (*)());
void dispatchGuiEvent(puObject*,int);
void guiClickCallback(puObject*);
void guiSubmitCallback(puObject*);
void guiChangeCallback(puObject*);
void setGuiTimedMessage(const char*,std::uint32_t);
void playGuiAcceptSound();
void playGuiCancelSound();
void playGuiDisabledSound(puObject*);
const char* getGuiEventHandler();
bool guiLayoutHasEmptyExtent(const int* bounds);
}
