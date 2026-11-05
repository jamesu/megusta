#include "input.h"
#include <cstring>
namespace megusta {
GameInputFrame gameInput{};
InputState gameInputState{gameInput};
void InputState::clearFrame() { joystickAxisCount=0; liveMouse.y=0; liveMouse.x=0; initialize(); }
void InputState::initialize() {
    joystickButtons=0; heldMouseButtons=0; mouseWrite=0; mouseRead=0; keyWrite=0; keyRead=0;
    frame.keyCode=0; frame.modifiers=0; rawModifiers=0; frame.previousButtons=0; frame.buttons=0;
    std::memset(keyDown,0,sizeof(keyDown));
}
void InputState::readFrame(bool sceneActive) {
    frame.previousButtons=frame.buttons; frame.buttons=0; frame.modifiers=0;
    if (keyRead==keyWrite) frame.keyCode=0;
    else { frame.keyCode=keyQueue[keyRead]; keyRead=(keyRead+1)&0x3fu; }
    if (rawModifiers&4u) frame.modifiers |= 1u;
    if (rawModifiers&2u) frame.modifiers |= 2u;
    if (rawModifiers&1u) frame.modifiers |= 4u;
    if (mouseRead==mouseWrite) { frame.mouse=liveMouse; frame.buttons |= heldMouseButtons; }
    else { frame.mouse=mouseQueue[mouseRead]; frame.buttons |= mouseButtonQueue[mouseRead]; mouseRead=(mouseRead+1)&7u; }
    frame.buttons |= joystickButtons;
    if (frame.joystickAxes[0]>0.2) frame.buttons |= 0x800000u;
    if (frame.joystickAxes[0]<-0.2) frame.buttons |= 0x400000u;
    if (frame.joystickAxes[1]>0.2) frame.buttons |= 0x200000u;
    if (frame.joystickAxes[1]<-0.2) frame.buttons |= 0x100000u;
    if (frame.joystickAxes[2]>0.2) frame.buttons |= 0x80000u;
    if (frame.joystickAxes[2]<-0.2) frame.buttons |= 0x40000u;
    if (frame.joystickAxes[3]>0.2) frame.buttons |= 0x20000u;
    if (frame.joystickAxes[3]<-0.2) frame.buttons |= 0x10000u;
    if (sceneActive) {
        if (keyDown[0x164]) frame.buttons |= 0x8000u;
        if (keyDown[0x166]) frame.buttons |= 0x2000u;
        if (keyDown[0x165]) frame.buttons |= 0x1000u;
        if (keyDown[0x167]) frame.buttons |= 0x4000u;
    }
}
void InputState::clearKey(std::uint32_t key) { if (key<0x16eu) keyDown[key]=0; }
void InputState::pressKey(std::uint32_t key,const InputServices& services) {
    if (key<0x16eu) keyDown[key]=1;
    rawModifiers=services.activeModifiers(services.context);
    keyQueue[keyWrite]=key; keyWrite=(keyWrite+1)&0x3fu;
    if (keyWrite==keyRead) keyRead=(keyRead+1)&0x3fu;
}
void InputState::setJoystick(std::uint32_t buttons,int count,const float* axes,const InputServices& services) {
    joystickButtons=buttons; joystickAxisCount=count;
    // The recovered loop writes the reported axis count without checking it.
    for (int i=0;i<count;++i) frame.joystickAxes[i]=axes[i];
}
void InputState::setMousePosition(int x,int y) { liveMouse.x=x; liveMouse.y=y; }
void InputState::setMouseButton(int button,int state,int x,int y,const InputServices& services) {
    rawModifiers=services.activeModifiers(services.context); setMousePosition(x,y);
    std::uint32_t mask;
    if (button==0) mask=0x1000000u; else if (button==1) mask=0x4000000u;
    else if (button==2) mask=0x2000000u; else return;
    std::uint32_t next;
    if (state==0) next=heldMouseButtons|mask;
    else if (state==1) next=heldMouseButtons&~mask;
    else return;
    bool changed=heldMouseButtons!=next; heldMouseButtons=next;
    if (changed) {
        mouseQueue[mouseWrite]=liveMouse; mouseButtonQueue[mouseWrite]=next;
        mouseWrite=(mouseWrite+1)&7u; if (mouseWrite==mouseRead) mouseRead=(mouseRead+1)&7u;
    }
}
}
