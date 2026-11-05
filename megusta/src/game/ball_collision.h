#pragma once
#include "ball.h"
#include "scene_grid.h"
namespace megusta {
// The original collision routines share one mutable sweep and result workspace.
class BallCollisionWorkspace {
public:
    BallContactResult result;
    Point3F start, end, delta, direction;
    float length, backoffFraction;
    SceneTriangleRecord triangle;
    void setSweep(const Point3F& from, const Point3F& to);
    long double distanceToEdgeAtFraction(const Point3F& first, const Point3F& second, float fraction);
    bool testTriangleInteriorOrEdge(const Point3F& point, const SceneTriangleServices& services);
    bool testSweptSphereTriangle(SceneObjectCollection& objects);
    bool castSweptBall(SceneObjectCollection& objects, SceneGridIndex& grid,
                       const Point3F& from, const Point3F& to, BallContactResult& out);
    bool castBallAgainstTriangle(SceneObjectCollection& objects, int index,
                                 const Point3F& from, const Point3F& to, BallContactResult& out);
    bool ballOverlapsGridObject(SceneObjectCollection& objects, SceneGridIndex& grid,
                                int objectIndex, const Point3F& position);
    void backOffBallContact(SceneObjectCollection& objects, SceneGridIndex& grid,
                            const Point3F& from, const Point3F& to, BallContactResult& inout);
};
extern BallCollisionWorkspace ballCollision;
struct BallRestContext {
    SceneObjectCollection& objects;
    SceneGridIndex& grid;
    BallCollisionWorkspace& collision;
    void* playerContext;
    int (*strokeAndPenaltyCount)(void*, int playerIndex);
    void (*historyInvariantFailure)(void*);
};
BallRestServices makeBallRestServices(BallRestContext& context);
struct BallPhysicsServices {
    SceneObjectCollection& objects;
    SceneGridIndex& grid;
    BallCollisionWorkspace& collision;
    BallRestServices& rest;
    const float* acceleration;
    // Recovered strings at 004f3788, 00504368 and 0050436c are respectively
    // "0", "321" and "123". Ball_resolveSceneImpact interprets each digit as
    // a collision-recovery opcode and concatenates them in a 12-byte buffer.
    const char* overlapRecovery;
    const char* slowRecovery;
    const char* fastRecovery;
    void* context;
    void (*playBallImpact)(void*, const float* position);
    void (*playObjectImpact)(void*, SceneObjectRecord&, const float* position, const float* normalVelocity);
    void (*dispatchTerminalEvent)(void*);
};
}
