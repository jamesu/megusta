#include "application_input.h"
#include "console.h"
#include "gui.h"
#include "pu.h"
namespace megusta {
namespace {
ApplicationInput& host(void* p) { return *static_cast<ApplicationInput*>(p); }
std::uint32_t modifiers(void* p) { return host(p).platform.activeModifiers(); }
int joystick(void* p) { return host(p).platform.systemProperty(3); }
void buttons(void* p,std::uint32_t value) { host(p).console.handleButtonMask(value); }
void special(void* p,int value) { host(p).console.handleSpecialKey(value); }
void character(void* p,std::uint8_t value) { host(p).console.handleCharacter(static_cast<char>(value)); }
bool visible(void* p) { return host(p).console.visible!=0; }
bool opening(void* p) { return host(p).console.opening!=0; }
void disableWarp(void* p) { host(p).gui.warpCursorOnTab=false; }
void mouseButton(void*,int button,int state,int x,int y) { puMouse(button,state,x,y); }
void mouseMotion(void*,int x,int y) { puMouse(x,y); }
void keyboard(void*,std::uint32_t key,int state) { puKeyboard(static_cast<int>(key),state); }
}
ApplicationInput::ApplicationInput(platform::Runtime& runtime,Console& output,guiSystem& system,
        InputState& input,DriverViewport& viewport,Hole& hole,PlayersManager& players,
        GameConfigRuntime& config,DriverScriptServices& scripts)
    : platform(runtime),console(output),gui(system),modifiers{this,megusta::modifiers},
      services{input,modifiers,viewport,hole,players,config,scripts,this,joystick,buttons,
               special,character,visible,opening,disableWarp,mouseButton,mouseMotion,keyboard} {}
}
