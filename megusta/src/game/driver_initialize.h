// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "startup.h"
#include "balls.h"
#include "putter.h"
#include "putt_guide.h"
#include "console.h"
#include "gui_assets.h"
#include "gui_runtime.h"
#include "scene_runtime.h"
#include "powerups.h"
#include "game_shape_adapter.h"
#include "engine/movie_playback.h"
#include "engine/runtime_adapters.h"
#include "room_client.h"
#include "sounds.h"
#include "platform/game_bridge.h"
class fntTexFont;
class ssgRoot;
namespace megusta {
class guiSystem;
namespace bindings { struct GameVariables; }
// Borrowed shared state; font/root pointers are the original runtime-owned
// objects, not copies created for a binding or an ABI wrapper.
struct DriverInitialization {
    GameConfig& config;
    StartupState& startup;
    Console& console;
    HoleFrameClock& clock;
    BallsManager& balls;
    Putter& putter;
    PuttGuide& guide;
    InputState& input;
    PlayersManager& players;
    Random& firstRandom;
    Random& secondRandom;
    fntTexFont*& primaryFont;
    fntTexFont*& consoleFont;
    fntTexFont*& secondaryFont;
    GuiWidgetResources& widgets;
    ResourceRuntime& resources;
    Sounds& sounds;
    guiSystem& gui;
    GuiRuntime& guiRuntime;
    SceneLoadState& sceneLoading;
    PowerUpsRuntime& powerUpsRuntime;
    GameShapeRuntime& shapes;
    std::uint32_t& softwareCursorTexture;
    engine_movie::MovieHost movieHost{};
    bindings::GameVariables& variables;
    iRoomClient*& roomClient;
    platform::DriverPlatformBinding& callbacks;
    void* context;
    engine_runtime::ExitHandler (*installExitHandler)(void*);
    void (*selectDefaultSceneLoader)(void*); // LAB_00462430, not forced-pix loader.
    void (*bindRoomClient)(void*,iRoomClient&)=nullptr;
};
}
