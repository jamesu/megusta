// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "scene_object.h"
#include "task.h"
namespace megusta {
class Ball;
class Random;
class Player;
struct BallPhysicsServices;
struct PlayerServices;
struct BallContactResult {
    int kind;
    float sweepParameter, position[3], normal[3];
    int objectIndex, triangleIndex;
    Ball* otherBall;
};
struct BallContactEvent { int objectIndex; float position[3]; Task* task; };
struct BallRestHistoryEntry {
    std::uint32_t flags;
    float position[3], normal[3];
    int supportObjectIndex, strokeAndPenaltyCount;
};
struct BallRestServices {
    void* context;
    bool (*castSweptBall)(void*, const Point3F&, const Point3F&, BallContactResult&);
    void (*backOffBallContact)(void*, const Point3F&, const Point3F&, BallContactResult&);
    int (*strokeAndPenaltyCount)(void*, int playerIndex);
    void (*historyInvariantFailure)(void*);
};
struct BallGameplayServices {
    void* context;
    Player* (*playerByIndex)(void*, int);
    Player* (*currentPlayer)(void*);
    void (*restorePlayerHeading)(void*);
    void (*recordStroke)(void*);
    void (*playStrikeSound)(void*, const float* position);
    std::uint32_t (*timeMilliseconds)(void*);
    void (*serviceNetworkEnd)(void*);
};
class BallTrailSlot {
public:
    ssgTransform* transform;
    ssgSelector* selector;
    int frame;
    bool active;
    float position[3];
    void initialize();
    void initializeModels(ssgEntity** models, unsigned count, ssgBranch* root);
    void start(sgMat4 matrix);
    void reset();
    void advance();
};
// Shared game ball record. Additional fields and methods are recovered as their
// call sites are implemented; this definition is not a VC6 layout contract.
class Ball {
public:
    void updateRenderModels();
    long double nearestCup(SceneObjectCollection&,const class PuttMap&) const;
    Player* getPlayer(const PlayerServices& services) const;
    void setColour(int colour, void (*fatal)());
    int playerIndex;
    std::uint32_t flags, turnFlag;
    int lastContactObjectIndex;
    float position[3], initializationVector[3], nextPosition[3], groundPosition[3];
    int colorModelIndex;
    float rollingRotation[9];
    ssgBaseTransform* model;
    BallTrailSlot trail[4];
    int trailHead;
    ssgEntity* specialModel[3];
    ssgAnim* specialAnimator[3];
    std::uint32_t movingStepCount;
    float normalVelocity[3], velocity[3], previousVelocity[3];
    BallContactResult contact, rollingContact;
    std::uint32_t contactEventCount;
    BallContactEvent contactEvents[4];
    std::uint32_t smallMotionStepCount, restHistoryIndex;
    BallRestHistoryEntry restHistory[4];
    void resetTrail();
    void resetMotionState();
    void setLocation(const float* location);
    void setVelocity(const float* value);
    void resetOffCourse();
    bool findRestingLocation(const BallRestServices& services);
    bool snapToSurface(const float* location, const BallRestServices& services);
    void rest(const BallRestServices& services);
    void advanceRestHistory(const BallRestServices& services);
    bool restoreRestHistory();
    bool lost(const BallGameplayServices& services);
    void strike(const Point3F& direction, const BallGameplayServices& services);
    bool isRolling() const;
    void decomposeContactVelocity(const float* normal);
    void rescaleContactSpeed(float sweepParameter, const float* acceleration);
    bool testMotionThresholds() const;
    bool cacheRestingContact(SceneObjectCollection& objects);
    void queryMotionContacts(BallPhysicsServices& services);
    bool resolveSceneImpact(BallPhysicsServices& services);
    void beginPhysicsStep(BallPhysicsServices& services);
    void resolveContactResponse(BallPhysicsServices& services);
    void updateRollingOrientation();
    void updateTrail();
    bool resolveBallCollision(Ball& other, Random& random);
    void setInCup();
    void clearContactTasks();
    void queueContactTask(const BallContactEvent& event);
    void dispatchContactTasks(TaskRuntime& runtime);
    void noteSceneContact(const BallContactResult& result, SceneObjectCollection& objects);
    SceneObjectRecord* lastTouch(SceneObjectCollection& objects) const;
};
extern float gameZeroVector[3];
void addVectorInPlace(Point3F& value, const Point3F& delta);
void applySpeedLoss(float* velocity, float loss);
}
