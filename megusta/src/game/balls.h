#pragma once
#include "ball_collision.h"
#include "random.h"
namespace megusta {
class PlayersManager;
class OpenGLAdapter;
struct BallsRenderState {
    SceneObjectCollection& objects;
    SceneGridIndex& grid;
    BallCollisionWorkspace& collision;
    PlayersManager& players;
    OpenGLAdapter& gl;
    const float* viewMatrix;
    std::uint32_t debugBallList=0;
};
void bindBallsRenderState(BallsRenderState&);
class BallsManager;
struct BallsStepServices {
    BallPhysicsServices& physics;
    TaskRuntime& tasks;
    const std::uint32_t& playerCount;
    void* context;
    Random& collisionRandom;
    bool (*isCurrentPlayerBall)(void*,const Ball&);
    // Performs the original player lookup and acts only if a player exists.
    void (*setTargetStrokeCount)(void*,Ball&);
};
class BallsManager {
public:
    std::uint32_t shadowTexture;
    Ball balls[4];
    Ball* currentBall;
    std::uint32_t collisionSeen;
    void initialize(std::uint32_t (*bindShadowTexture)(void*), void* context);
    void draw(BallsRenderState&);
    void drawGroundShadows(BallsRenderState&);
    void loadHoleModels(BallsRenderState&);
    void releaseHoleModels(BallsRenderState&);
    void stepPhysics(BallsStepServices& services);
    void placeAllAtTee(SceneObjectRecord& tee, const BallRestServices& services);
    void forgetRemovedContactObject(int objectIndex);
    void triggerObjectContactTask(int objectIndex, SceneObjectCollection& objects,
                                  SceneGridIndex& grid, BallCollisionWorkspace& collision,
                                  TaskRuntime& runtime);
};
extern BallsManager gameBalls;
}
