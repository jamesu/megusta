// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include "player_ai.h"
#include "game_shape_adapter.h"
class ssgTransform;
namespace megusta {
class Ball;
class BallsManager;
class Task;
struct TaskRuntime;
struct BallRestServices;
struct PlayerServices;
class Putter;
class Camera;
class iRoomClient;
class PowerUps;
struct CameraFrameServices;
struct PutterAnimationServices;
struct PutterTurnRuntime;
struct GameInputFrame;
struct PlayerInputContext;
// The player's three inventory entries are not world PowerUp objects. The
// saved bodies address only the ID at each 0xc0-byte entry stride; the
// remaining storage is opaque. Keeping this as inert storage avoids running
// SpatialObject default initializers (identity transform/radius) in slots
// that the original player-record initialization zeroed.
struct PlayerPowerUpSlot {
    std::uint32_t definitionId;
    std::uint8_t unresolvedStorage[188];
};
static_assert(sizeof(PlayerPowerUpSlot)==0xc0);
// Partial player representation, shared by ball gameplay services. The
// remaining player state is added as its behavior is recovered.
class Player {
public:
    std::uint32_t flags;
    char name[32], modelTag[8];
    int playerIndex, strokeCount, penaltyCount;
    int points; // Player_setPoints/getPoints, original PlayerRecord offset 60.
    bool lastStrokePenaltyFlag, isHuman;
    std::uint32_t puttDeadlineMs;
    int skinIndex, playClock, selectedPowerUpSlot, controlMode;
    float accuracy, controlAttribute, luck;
    bool useMouse, controlFlag8d, controlFlag8e, leftHanded, powerUpActivationPending;
    std::uint32_t controlOption94, controlOption98, controlOption9c, inputDeviceId;
    PlayerPowerUpSlot powerUps[3];
    PlayerAI ai;
    GameShapeAdapter shapeAdapter;
    float characterTransform[4][4];
    Ball* getBall(BallsManager& balls) const;
    void setSkin(int index,const PlayerServices& services);
    bool isDone(BallsManager& balls) const;
    void takeMulligan(class PlayersManager& players,BallsManager& balls,const PlayerServices& services);
    void addPowerUpUnchecked(std::uint32_t id, const PlayerServices& services);
    std::uint32_t addPowerUp(std::uint32_t id, const PlayerServices& services);
    void usedPowerUp(const PlayerServices& services);
    void selectPreviousPowerUp(const PlayerServices& services);
    void tryActivatePowerUp(Random& random,const PlayerServices& services);
};
struct PlayerServices {
    void* context;
    void (*fatal)(void*);
    bool (*sendPickupPowerUp)(void*,std::uint32_t);
    void (*sendActivatePowerUp)(void*);
    void (*playButtonSound)(void*);
    GameConfig& config;
    Random& random;
    bool (*networkInGame)(void*);
    void (*finishPutt)(void*);
    void (*logSetCurrent)(void*,const char*);
    bool (*canProgressActivePutt)(void*);
    bool (*acknowledgeFrameGate)(void*);
    void (*reportMissingEvent)(void*,int line);
    void (*releasePlayerShape)(void*,Player&);
    void (*unloadSoundGroup)(void*,unsigned group);
    void (*loadPlayerShape)(void*,Player&,unsigned detail,bool enabled);
    const char* const* voiceNames; // Ten entries, including nulls.
    void (*preloadVoice)(void*,const char* modelTag,const char* voice,unsigned group);
    int (*inputDeviceEnabled)(void*);
    std::uint32_t (*inputDeviceCount)(void*);
    void (*preloadButtonSound)(void*,int group);
    bool (*isEditing)(void*);
    std::uint32_t (*timeMilliseconds)(void*);
    void (*serviceNetworkEnd)(void*);
    void (*setPlayerSkin)(void*,Player&,int);
    PutterTurnRuntime* putterTurn=nullptr;
    PowerUps* powerUps=nullptr;
    int* powerUpDirty=nullptr;
    iRoomClient** client=nullptr;
};
struct PutterTurnRuntime {
    Putter* putter=nullptr;
    PutterAnimationServices* animation=nullptr;
    Camera* camera=nullptr;
    CameraFrameServices* cameraFrame=nullptr;
    GameInputFrame* input=nullptr;
    iRoomClient** client=nullptr;
    std::uint32_t* fixedTick=nullptr;
    std::uint32_t* taskTick=nullptr;
    int* editorMode=nullptr;
};
struct InputMousePosition { std::int32_t x,y; };
struct GameInputFrame {
    InputMousePosition mouse;
    std::uint32_t buttons,previousButtons,modifiers,keyCode;
    float joystickAxes[4];
};
struct PlayerInputContext {
    GameInputFrame& input;
    const float (&teePositions)[3][3];
    const float* smoothedAim;
    BallsManager& balls;
    TaskRuntime& tasks;
    const BallRestServices& rest;
    const PlayerServices& services;
};
class PlayersManager {
public:
    PlayersManager() = default;
    virtual ~PlayersManager() { reset(); }
    int turnState;
    Task* eventTasks[4];
    int teeIndex, displayedStrokeCount;
    Player* playerStorage=nullptr;
    std::uint32_t playerCount;
    Player* currentPlayer;
    bool holeFlag;
    std::uint32_t lastProgressTick, progressInterval, messageStartTick, messageDuration;
    char message[4096];
    void reset();
    void drawOverlay(struct PlayerOverlayServices&);
    Player* add(const char* name,const char* modelTag,const PlayerServices& services);
    Player* get(std::uint32_t index,const PlayerServices& services);
    void showMessage(const char* text,std::uint32_t duration,std::uint32_t tick);
    void setCurrent(Player* player,BallsManager& balls,const PlayerServices& services);
    Player* findNext(Player* player);
    Player* findNextOrFirst(Player* player);
    void beginTurn(BallsManager& balls,const PlayerServices& services);
    bool isActive() const;
    void recordStroke(BallsManager& balls,const BallRestServices& rest);
    void markReadyToAdvance();
    bool allBallsReady(BallsManager& balls,const PlayerServices& services);
    void handleInput(PlayerInputContext& context);
    void release(const PlayerServices& services);
    void initializeHole(std::uint32_t tick,BallsManager& balls,TaskRuntime& tasks,const PlayerServices& services);
    void advanceTurnIfReady(std::uint32_t tick,BallsManager& balls,const PlayerServices& services);
    void setEventHandler(int slot,Task* task);
    void dispatchTerminalEvent(BallsManager& balls,TaskRuntime& tasks,const struct BallGameplayServices& gameplay);
    void startNextPlayerTask(BallsManager& balls,TaskRuntime& tasks,const PlayerServices& services);
    void startNextHoleTask(TaskRuntime& tasks,const PlayerServices& services);
};
extern PlayersManager gamePlayers;
bool isBallPositionValid(float x,float y,const float* teePosition);
}
