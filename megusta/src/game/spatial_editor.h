#pragma once
#include "native_game_helpers.h"
#include "camera.h"
namespace megusta {
class PowerUps;
class iRoomClient;
class GameConfigRuntime;
class SpatialObject;
struct SpatialEditorServices {
    Camera& camera;
    CameraFrameServices& cameraFrame;
    PowerUps& powers;
    int& editorMode;
    int& moveStep;
    bool debugKeys,networkInGame;
    const char* mapName;
    const char* spatialExtension;
};
class SpatialEditor {
public:
    void processInput(SpatialEditorServices&);
    void showHelp();
    std::uint32_t repeatCounter=0;
    SpatialObject* relationshipSource=nullptr;
};
struct WireDebugServices {
    SceneObjectCollection& objects;
    SceneGridIndex& grid;
    GameInputFrame& input;
    bool debugKeys,networkInGame;
    const char* const* modeNames;
    ssgCallback allCallback,triangleCallback,candidatesCallback;
};
void showWireSelection(const WireDebugState&,WireDebugServices&);
void handleWireInput(WireDebugState&,WireDebugServices&);
}
