#include "gui_runtime.h"
#include "gui_list.h"
#include "gui_widgets.h"
#include "gui_tooltip.h"
#include "gui_shell.h"
#include "gui_render.h"
#include "room_client.h"
#include "fnt.h"
#include "../engine/platform_gl.h"
#include "engine/opengl_adapter.h"
#include "engine/string_services.h"
#include "pu.h"
#include "ssg.h"
#include <cstring>
#include <cstdio>
#include <stdexcept>
namespace megusta {
GuiRuntime* guiRuntime=nullptr;
namespace {
// Values and order exported from the executable's tables at 004eae00 and
// 004ead10. A host can still override either list, but the normal path uses
// the original names and DirectInput/keyboard codes.
const GuiNameCode recoveredKeyNames[]={
    {"F1",0x101},{"F2",0x102},{"F3",0x103},{"F4",0x104},
    {"F5",0x105},{"F6",0x106},{"F7",0x107},{"F8",0x108},
    {"F9",0x109},{"F10",0x10a},{"F11",0x10b},{"F12",0x10c},
    {"LSHIFT",0x11a},{"RSHIFT",0x11b},{"LCONTROL",0x11c},{"RCONTROL",0x11d},
    {"LALT",0x11e},{"RALT",0x11f},{"LEFT",0x164},{"UP",0x165},
    {"RIGHT",0x166},{"DOWN",0x167},{"PAGE_UP",0x168},{"PAGE_DOWN",0x169},
    {"HOME",0x16a},{"END",0x16b},{"INSERT",0x16c},{"KEYPAD5",0x16d},{nullptr,0}
};
const GuiNameCode recoveredButtonNames[]={
    {"L1",0x00000004},{"L2",0x00000001},{"L3",0x00000200},
    {"R1",0x00000008},{"R2",0x00000002},{"R3",0x00000400},
    {"RUP",0x00000010},{"RDOWN",0x00000040},{"RLEFT",0x00000080},
    {"RRIGHT",0x00000020},{"START",0x00000800},{"SELECT",0x00000100},
    {"LUP",0x00001000},{"LDOWN",0x00004000},{"LLEFT",0x00008000},
    {"LRIGHT",0x00002000},{"ALUP",0x00100000},{"ALDOWN",0x00200000},
    {"ALLEFT",0x00400000},{"ALRIGHT",0x00800000},{"ARUP",0x00010000},
    {"ARDOWN",0x00020000},{"ARLEFT",0x00040000},{"ARRIGHT",0x00080000},
    {"BUTTON_A",0x01000000},{"BUTTON_B",0x02000000},
    {"LBUTTON",0x01000000},{"MBUTTON",0x04000000},{"RBUTTON",0x02000000},{nullptr,0}
};
}
void bindGuiRuntime(GuiRuntime& runtime) {
    if (!runtime.keyNames) runtime.keyNames=recoveredKeyNames;
    if (!runtime.buttonNames) runtime.buttonNames=recoveredButtonNames;
    guiRuntime=&runtime;
}
std::uint32_t lookupGuiNameCode(const GuiNameCode* table,const char* name) {
    if (name) for (;table->name;++table)
        if (!strings::compareInsensitive(name,table->name)) return table->code;
    return 0;
}
void guiBindings::initialize() {
    for (int i=0;i<5;++i) { buttonMasks[i]=0; buttonActions[i]=nullptr; keyActions[i]=nullptr; }
    level=0;
}
void guiBindings::release() {
    const int last=level;
    for (int i=0;i<=last;++i) { level=i; clear(); }
    level=0;
}
void guiBindings::add(const char* name,const char* action) {
    if (std::strlen(name)==1) { addCode(static_cast<unsigned char>(*name),action,false); return; }
    auto code=lookupGuiNameCode(guiRuntime->keyNames,name);
    if (code) { addCode(code,action,false); return; }
    code=lookupGuiNameCode(guiRuntime->buttonNames,name);
    if (code) addCode(code,action,true);
}
void guiBindings::remove(const char* name) {
    if (std::strlen(name)==1) { removeCode(static_cast<unsigned char>(*name),false); return; }
    auto code=lookupGuiNameCode(guiRuntime->keyNames,name);
    if (code) { removeCode(code,false); return; }
    code=lookupGuiNameCode(guiRuntime->buttonNames,name);
    if (code) removeCode(code,true);
}
void executeGuiBindingAction(const char* action) {
    auto& runtime=*guiRuntime;
    if (*action=='=') {
        if (!strings::compareInsensitive(action+1,"CLICK")) {
            if (!runtime.clickPending) {
                runtime.mouse(runtime.context,0,0,runtime.input.mouse.x,runtime.input.mouse.y);
                runtime.clickStart=runtime.fixedTick(runtime.context);
                runtime.clickDuration=5; runtime.clickPending=true;
            }
        } else {
            auto code=lookupGuiNameCode(runtime.buttonNames,action+1);
            if (code) runtime.buttonEdges|=code;
        }
    } else runtime.executePython(runtime.context,action);
}
void guiBindings::dispatchInput() {
    auto& runtime=*guiRuntime;
    runtime.buttonEdges=~runtime.input.previousButtons & runtime.input.buttons;
    auto key=runtime.input.keyCode;
    if (key>'`' && key<'{') key-=0x20;
    if (runtime.input.modifiers&1) key=0;
    if (key) for (auto* node=keyActions[level];node;node=node->next) {
        if (node->code==key) { executeGuiBindingAction(node->action); break; }
    }
    if (buttonMasks[level]&runtime.buttonEdges)
        for (auto* node=buttonActions[level];node;node=node->next) {
            if (node->code&runtime.buttonEdges) { executeGuiBindingAction(node->action); break; }
        }
    if (runtime.clickPending && runtime.clickDuration<=runtime.fixedTick(runtime.context)-runtime.clickStart) {
        runtime.mouse(runtime.context,0,1,runtime.input.mouse.x,runtime.input.mouse.y);
        runtime.clickPending=false;
    }
}
void repeatGameCallback(std::uint32_t,std::uint32_t,int count,void (*callback)()) {
    for (int i=0;i<count;++i) callback();
}
void dispatchGuiEvent(puObject* widget,int event) {
    auto* element=static_cast<guiElement*>(widget->getUserData());
    if (element->eventHandlers[event]) {
        auto& runtime=*guiRuntime;
        runtime.system.currentElement=element;
        runtime.executePython(runtime.context,element->eventHandlers[event]);
        runtime.system.currentElement=nullptr;
    }
}
void guiClickCallback(puObject* widget) { dispatchGuiEvent(widget,1); }
void guiSubmitCallback(puObject* widget) { dispatchGuiEvent(widget,2); }
void guiChangeCallback(puObject* widget) { dispatchGuiEvent(widget,0); }
std::uint32_t* guiSystem::initialize() {
    bindings.initialize(); warpCursorOnTab=true; backgroundTexture=0;
    backgroundTextures=nullptr; frameDelta=0; previewRoot=nullptr; pressedElement=nullptr;
    return &backgroundTexture;
}
void guiSystem::refreshAnimationFlag() {
    hasTabElements=false;
    for (int i=0;i<elementCount && !hasTabElements;++i) hasTabElements=elements[i]->tabState>=0;
}
bool guiSystem::isWidgetInputAllowed(puObject* widget) const {
    if (hasInterface) {
        int first=0;
        for (int i=0;i<elementCount;++i) if (elements[i]->type==0) first=i+1;
        for (int i=first;i<elementCount;++i) if (elements[i]->widget==widget) return true;
        return false;
    }
    return !pressedElement || pressedElement->widget==widget;
}
void guiSystem::updateFrameDelta() {
    auto& runtime=*guiRuntime;
    auto now=runtime.milliseconds(runtime.context);
    frameDelta=static_cast<std::int32_t>(now-runtime.previousFrameMs)*0.001f;
    runtime.previousFrameMs=now;
    if (frameDelta>=0.1f) frameDelta=0.1f;
    else if (frameDelta<=0.0f) frameDelta=0.0f;
}
void guiSystem::updateInteraction(bool debugKeys,bool networkInGame,GuiDebugState& debug) {
    bindings.dispatchInput(); updateGuiTooltipHover();
    if (debugKeys && !networkInGame) {
        if (guiRuntime->input.keyCode==0x10a) debug.drawViewportBorder=!debug.drawViewportBorder;
        else if (guiRuntime->input.keyCode==0x10b) {
            debug.renderMode^=3;
            debug.renderOptionLow=(debug.renderMode&1)!=0;
            debug.renderOptionHigh=((~(debug.renderMode>>1))&1)!=0;
        }
    }
}
void drawGuiChatOverlay(iRoomClient& client,bool activeScene,const char* mapName,const GuiDebugState& debug) {
    auto& runtime=*guiRuntime;
    if (client.inGame() && (activeScene || !strings::compareInsensitive(mapName,"result"))) {
        fntRenderer renderer; renderer.setFont(guiFonts.console); renderer.setPointSize(20); renderer.begin();
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        float y=runtime.screenHeight*140.0f*0.0020833334f;
        const int count=client.getNumSaid(); const auto now=runtime.milliseconds(runtime.context);
        for (int line=0,index=count-1;line<count && line<10;++line,--index) {
            const char* text=client.getSaid(index);
            const float age=static_cast<float>(static_cast<std::int32_t>(now-client.getSaidMetadata(index)));
            const float alpha=age<3000?1.0f:1.0f-(age-3000)*0.0002f;
            if (age<8000) {
                renderer.start2f(11,y); glColor4f(0,0,0,alpha*0.9f); renderer.puts(text);
                renderer.start2f(9,y+2); glColor4f(1,1,1,alpha*0.9f); renderer.puts(text); y+=24;
            }
        }
        renderer.end();
    } else if (runtime.messageActive) {
        if (runtime.fixedTick(runtime.context)-runtime.messageStart<runtime.messageDuration) {
            fntRenderer renderer; renderer.setFont(guiFonts.console); renderer.setPointSize(12); renderer.begin();
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            renderer.start2f(11,10); glColor3f(0,0,0); renderer.puts(runtime.message);
            renderer.start2f(10,11); glColor3f(1,0,0); renderer.puts(runtime.message); renderer.end();
        } else runtime.messageActive=false;
    }
    if (debug.drawViewportBorder) {
        // Match ssgSupport_disableTextureStates_role: clear the three SSG
        // enable bits and issue the original engine-token disables. Forcing
        // the SSG basic state also enables culling and changes other state.
        auto* state=ssgGetCurrentContext()->getState();
        constexpr unsigned disabledSsgEnables=(1u<<SSG_GL_TEXTURE_EN)|
            (1u<<SSG_GL_COLOR_MATERIAL_EN)|(1u<<SSG_GL_LIGHTING_EN);
        state->clearEnableFlags(disabledSsgEnables);
        auto& gl=runtime.renderer->gl;
        gl.disable(0x4a); gl.disable(0x76); gl.disable(0x67); gl.disable(0x21);
        glColor3f(1,1,1); glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        glBegin(GL_QUADS); glVertex2i(8,32); glVertex2i(runtime.screenWidth-8,32);
        glVertex2i(runtime.screenWidth-8,runtime.screenHeight-32); glVertex2i(8,runtime.screenHeight-32);
        glEnd(); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    }
}
ssgRoot* guiSystem::getRoot() { if (!previewRoot) previewRoot=new ssgRoot; return previewRoot; }
void setGuiTimedMessage(const char* text,std::uint32_t duration) {
    auto& runtime=*guiRuntime;
    runtime.messageActive=true; runtime.messageStart=runtime.fixedTick(runtime.context);
    runtime.messageDuration=duration; std::strcpy(runtime.message,text);
}
void playGuiAcceptSound() {
    auto& runtime=*guiRuntime;
    if (!runtime.soundPlaying(runtime.context,"BUTTON3",0) && !runtime.soundPlaying(runtime.context,"BUTTON2",0))
        runtime.playSound(runtime.context,"BUTTON3",0);
}
void playGuiCancelSound() {
    auto& runtime=*guiRuntime;
    if (!runtime.soundPlaying(runtime.context,"BUTTON2",0)) runtime.playSound(runtime.context,"BUTTON2",0);
}
void playGuiDisabledSound(puObject* widget) {
    auto* element=static_cast<guiElement*>(widget->getUserData());
    if (!(element->flags&1)) guiRuntime->playSound(guiRuntime->context,"BUTTON",0);
}
const char* getGuiEventHandler() { return nullptr; }
bool guiLayoutHasEmptyExtent(const int* bounds) { return bounds[2]<bounds[0] || bounds[3]<bounds[1]; }
void guiElement::setWriteOnlyAttr(int which,const char* text) {
    int parsed[3]={0,0,0};
    std::sscanf(text,"%d,%d,%d",parsed,parsed+1,parsed+2);
    int w,h; widget->getSize(&w,&h);
    switch (which) {
    case 0: widget->setPosition(parsed[0],parsed[1]); widget->setSize(w,h); break;
    case 1: {int x,y;widget->getPosition(&x,&y);widget->setPosition(x+parsed[0],y+parsed[1]);widget->setSize(w,h);break;}
    case 2: if(parsed[0])widget->activate();else widget->greyOut();break;
    case 3: if(parsed[0])widget->reveal();else widget->hide();break;
    case 4: flags=static_cast<unsigned>(parsed[0]);break;
    case 5: widget->setSortPriority(parsed[0]);break;
    }
}
int guiElement::specialCmd(const char* command,const char* text,int a,int b,int c,int d) {
    if (type==19) return static_cast<GuiListButton*>(widget)->specialCommand(command,text,a,b,c,d);
    if (type==6 && height>=width) return static_cast<int>(static_cast<GuiVerticalSlider*>(widget)->specialCommand(command,a,b,c));
    if (type==18) return static_cast<GuiSceneButton*>(widget)->specialCommand(command);
    if (type==2) return static_cast<GuiText*>(widget)->specialCommand(command);
    if (type==4 || type==13) return static_cast<GuiToggleLozenge*>(widget)->specialCommand(command,a,b,c);
    if (type==14) return static_cast<GuiLozenge2*>(widget)->specialCommand(command,a,b,c);
    return 0;
}
void guiSystem::setCurrentTab(guiElement* element,bool warp) {
    tabElement=element;
    if (element && warp && warpCursorOnTab) {
        auto& runtime=*guiRuntime;
        runtime.input.mouse.x=element->width/2+element->x;
        runtime.input.mouse.y=runtime.screenHeight-element->height/2-element->y;
        runtime.warpCursor(runtime.context,runtime.input.mouse.x,runtime.input.mouse.y);
        runtime.enableCursorMotion(runtime.context);
        if (element->type==19) static_cast<GuiListButton*>(element->widget)->handleDirection(0,0);
    }
}
}
