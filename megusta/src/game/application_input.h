#pragma once
#include "driver.h"
#include "platform/runtime.h"
namespace megusta {
class Console;
class guiSystem;
// Application-owned adapters. The referenced native objects must outlive the
// platform event callbacks; no copied binding views or secondary input state.
class ApplicationInput {
public:
    ApplicationInput(platform::Runtime&,Console&,guiSystem&,InputState&,
                     DriverViewport&,Hole&,PlayersManager&,GameConfigRuntime&,
                     DriverScriptServices&);
    platform::Runtime& platform;
    Console& console;
    guiSystem& gui;
    InputServices modifiers;
    DriverInputServices services;
};
}
