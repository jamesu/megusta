#include "gameplay_runtime.h"
namespace megusta {
namespace {
GameplayRuntime& runtime(void* p) { return *static_cast<GameplayRuntime*>(p); }
Ball* currentBall(void* p) { return runtime(p).balls.currentBall; }
std::uintptr_t lastTouch(void* p,Ball* ball) {
    return reinterpret_cast<std::uintptr_t>(ball->lastTouch(runtime(p).objects));
}
void prepareDraw(void* p) { auto& r=runtime(p); r.host.prepareDraw(r.host.context); }
int strokeAndPenaltyCount(void* p,int index) {
    auto& r=runtime(p); Player* player=r.players.get(static_cast<std::uint32_t>(index),r.host.playerServices);
    return player->strokeCount+player->penaltyCount;
}
void fatalHistory(void* p) { auto& r=runtime(p); r.host.playerServices.fatal(r.host.playerServices.context); }
Player* playerByIndex(void* p,int index) {
    auto& r=runtime(p); return r.players.get(static_cast<std::uint32_t>(index),r.host.playerServices);
}
Player* currentPlayer(void* p) { return runtime(p).players.currentPlayer; }
void restoreHeading(void* p) { auto& r=runtime(p); r.host.restorePlayerHeading(r.host.context); }
void recordStroke(void* p) { auto& r=runtime(p); r.players.recordStroke(r.balls,r.rest); }
void strikeSound(void* p,const float* position) { auto& r=runtime(p); r.host.playStrikeSound(r.host.context,position); }
std::uint32_t timeMilliseconds(void* p) { auto& r=runtime(p); return r.host.timeMilliseconds(r.host.context); }
void serviceNetworkEnd(void* p) { auto& r=runtime(p); r.host.serviceNetworkEnd(r.host.context); }
void ballImpact(void* p,const float* position) { auto& r=runtime(p); r.host.playBallImpact(r.host.context,position); }
void objectImpact(void* p,SceneObjectRecord& object,const float* position,const float* normal) {
    auto& r=runtime(p); r.host.playObjectImpact(r.host.context,object,position,normal);
}
void terminalEvent(void* p) { auto& r=runtime(p); r.players.dispatchTerminalEvent(r.balls,r.tasks,r.gameplay); }
bool currentPlayerBall(void* p,const Ball& ball) {
    auto& r=runtime(p); return r.players.currentPlayer && r.players.currentPlayer==playerByIndex(p,ball.playerIndex);
}
void targetStrokeCount(void* p,Ball& ball) {
    auto& r=runtime(p); Player* player=playerByIndex(p,ball.playerIndex);
    if (player) player->ai.setTargetStrokeCount();
}
}
GameplayRuntime::GameplayRuntime(PlayersManager& playerManager,BallsManager& ballManager,TasksManager& taskManager,
        SceneObjectCollection& objectCollection,SceneGridIndex& sceneGrid,BallCollisionWorkspace& workspace,
        Random& random,GameplayHost& platform)
    : players(playerManager),balls(ballManager),objects(objectCollection),grid(sceneGrid),collision(workspace),
      collisionRandom(random),host(platform),
      tasks{this,taskManager,currentBall,lastTouch,prepareDraw,invokePythonTaskCallback,releasePythonTaskCallback},
      restContext{objects,grid,collision,this,strokeAndPenaltyCount,fatalHistory},rest(makeBallRestServices(restContext)),
      gameplay{this,playerByIndex,currentPlayer,restoreHeading,recordStroke,strikeSound,timeMilliseconds,serviceNetworkEnd},
      physics{objects,grid,collision,rest,host.acceleration,host.overlapRecovery,host.slowRecovery,host.fastRecovery,
              this,ballImpact,objectImpact,terminalEvent} {}
void GameplayRuntime::stepBalls() {
    BallsStepServices services{physics,tasks,players.playerCount,this,collisionRandom,currentPlayerBall,targetStrokeCount};
    balls.stepPhysics(services);
}
}
