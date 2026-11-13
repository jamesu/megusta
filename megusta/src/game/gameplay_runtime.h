// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "balls.h"
#include "player.h"
namespace megusta {
struct GameplayHost {
    PlayerServices& playerServices;
    void* context;
    void (*prepareDraw)(void*);
    void (*restorePlayerHeading)(void*);
    void (*playStrikeSound)(void*,const float*);
    void (*playBallImpact)(void*,const float*);
    void (*playObjectImpact)(void*,SceneObjectRecord&,const float*,const float*);
    std::uint32_t (*timeMilliseconds)(void*);
    void (*serviceNetworkEnd)(void*);
    const float* acceleration;
    const char* overlapRecovery;
    const char* slowRecovery;
    const char* fastRecovery;
};
// Owns adapters but not the referenced game state or host services. Retain for
// the entire duration of any scheduled callbacks and ball simulation.
class GameplayRuntime {
public:
    GameplayRuntime(PlayersManager&,BallsManager&,TasksManager&,SceneObjectCollection&,
                    SceneGridIndex&,BallCollisionWorkspace&,Random&,GameplayHost&);
    GameplayRuntime(const GameplayRuntime&)=delete;
    GameplayRuntime& operator=(const GameplayRuntime&)=delete;
    void stepBalls();
    PlayersManager& players;
    BallsManager& balls;
    SceneObjectCollection& objects;
    SceneGridIndex& grid;
    BallCollisionWorkspace& collision;
    Random& collisionRandom;
    GameplayHost& host;
    TaskRuntime tasks;
    BallRestContext restContext;
    BallRestServices rest;
    BallGameplayServices gameplay;
    BallPhysicsServices physics;
};
}
