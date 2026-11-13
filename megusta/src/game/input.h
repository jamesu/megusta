// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "player.h"
namespace megusta {
struct InputServices {
    void* context;
    std::uint32_t (*activeModifiers)(void*);
};
class InputState {
public:
    explicit InputState(GameInputFrame& frameValue):frame(frameValue) {}
    GameInputFrame& frame;
    std::uint32_t keyQueue[64]{};
    InputMousePosition mouseQueue[8]{}, liveMouse{};
    std::uint8_t keyDown[0x16e]{};
    std::uint32_t mouseButtonQueue[8]{},keyRead=0,keyWrite=0,mouseRead=0,mouseWrite=0;
    std::uint32_t heldMouseButtons=0,joystickButtons=0,rawModifiers=0;
    int joystickAxisCount=0;
    void clearFrame();
    void initialize();
    void readFrame(bool sceneActive);
    void clearKey(std::uint32_t key);
    void pressKey(std::uint32_t key,const InputServices& services);
    void setJoystick(std::uint32_t buttons,int count,const float* axes,const InputServices& services);
    void setMousePosition(int x,int y);
    void setMouseButton(int button,int state,int x,int y,const InputServices& services);
};
extern GameInputFrame gameInput;
extern InputState gameInputState;
}
