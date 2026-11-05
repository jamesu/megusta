#include "balls.h"
namespace megusta {
BallsManager gameBalls{};
void BallsManager::initialize(std::uint32_t (*bindShadowTexture)(void*),void* context) {
    for (int i=0;i<4;++i) {
        balls[i].playerIndex=i; balls[i].colorModelIndex=17;
        balls[i].initializationVector[0]=0; balls[i].initializationVector[1]=0; balls[i].initializationVector[2]=0;
    }
    shadowTexture=bindShadowTexture(context);
}
void BallsManager::stepPhysics(BallsStepServices& services) {
    for (unsigned i=0;i<services.playerCount;++i) { currentBall=&balls[i]; currentBall->beginPhysicsStep(services.physics); }
    for (unsigned i=0;i<services.playerCount;++i) {
        currentBall=&balls[i];
        for (unsigned j=i+1;j<services.playerCount;++j) {
            Ball& other=balls[j];
            if (currentBall->resolveBallCollision(other,services.collisionRandom)) {
                collisionSeen=1;
                if (services.isCurrentPlayerBall(services.context,*currentBall)) services.setTargetStrokeCount(services.context,other);
            }
        }
    }
    for (unsigned i=0;i<services.playerCount;++i) {
        currentBall=&balls[i]; currentBall->resolveContactResponse(services.physics);
        currentBall->dispatchContactTasks(services.tasks);
        currentBall->updateRollingOrientation(); currentBall->updateTrail();
    }
    currentBall=nullptr;
}
void BallsManager::placeAllAtTee(SceneObjectRecord& tee,const BallRestServices& services) {
    float* location=tee.getPosition();
    for (Ball& ball:balls) { currentBall=&ball; ball.setLocation(location); currentBall->lastContactObjectIndex=tee.objectIndex; currentBall->findRestingLocation(services); }
    currentBall=nullptr;
}
void Ball::setInCup() { flags |= 0x10u; }
void BallsManager::forgetRemovedContactObject(int objectIndex) {
    if (!objectIndex) return;
    for (Ball& ball:balls) {
        unsigned i=0;
        do { if (ball.restHistory[i].supportObjectIndex==objectIndex) ball.restHistory[i].supportObjectIndex=0; ++i; }
        while (i<=ball.restHistoryIndex);
        if ((ball.flags&1u) && !ball.restHistory[ball.restHistoryIndex].supportObjectIndex) {
            if (ball.restHistoryIndex) --ball.restHistoryIndex;
            ball.resetMotionState();
        }
    }
}
void BallsManager::triggerObjectContactTask(int index,SceneObjectCollection& objects,SceneGridIndex& grid,
                                             BallCollisionWorkspace& collision,TaskRuntime& runtime) {
    Task* saved=runtime.scheduledTasks.currentTask;
    if (index && objects.objects[index].contactTask) {
        runtime.scheduledTasks.currentTask=nullptr;
        for (Ball& ball:balls) {
            currentBall=&ball;
            if (!(ball.flags&10u) && collision.ballOverlapsGridObject(objects,grid,index,
                    {ball.position[0],ball.position[1],ball.position[2]})) {
                ball.contact.kind=1; ball.contact.sweepParameter=0;
                for (int axis=0;axis<3;++axis) { ball.contact.position[axis]=ball.position[axis]; ball.nextPosition[axis]=ball.position[axis]; }
                ball.contact.normal[0]=0; ball.contact.normal[1]=0; ball.contact.normal[2]=1;
                ball.contact.objectIndex=index; ball.contact.triangleIndex=0; ball.contact.otherBall=nullptr;
                objects.objects[index].contactTask->start(runtime);
            }
        }
        currentBall=nullptr;
    }
    runtime.scheduledTasks.currentTask=saved;
}
}
