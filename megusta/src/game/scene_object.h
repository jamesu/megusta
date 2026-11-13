// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "scene_triangle.h"
class ssgAnim;
namespace megusta {
class Sounds;
class Task;
struct SceneObjectRecord;
struct SceneObjectCollection;
struct SceneObjectAnimationServices {
    void* context;
    SceneObjectRecord* (*findByName)(void*, const char*);
    void (*forgetContactObject)(void*, int);
    void (*triggerContactTask)(void*, int);
};
// Partial game object: remaining fields are added as their methods are recovered.
struct SceneObjectRecord {
    const char* name;
    ssgEntity* entity;
    SceneTriangleRecord* triangles;
    int triangleCount;
    sgMat4 baseFrameTransform;
    sgMat4 frameTransform;
    bool transformDirty;
    std::uint32_t flags;
    std::uint32_t currentFrame;
    std::uint32_t frameCount;
    int objectIndex;
    ssgAnim* animator;
    sgBox localBounds;
    sgBox worldBounds;
    float localCenter[3];
    float worldPositionScratch[3];
    SceneObjectRecord* seeThruFriends[32];
    int seeThruFriendCount;
    float transparencyAmount;
    std::uint32_t transparencyAux;
    float coefficientOfRestitution;
    float coefficientOfFriction;
    char* impactSound;
    Task* contactTask;
    void refreshFrameTransform();
    float* getFrameTransform();
    float* getBaseFrameTransform();
    float* getPosition();
    void setWire(bool enabled, void (*applyWireDebug)());
    void setSound(const char* sound);
    void playImpactSound(const float*,const float*,Sounds&);
    void detectTransparency();
    void setAlpha(float transparency);
    void hide();
    void show();
    sgBox* updateWorldBounds();
    const float* getDirection();
    float getBallDistance(const float* ballPosition);
    void selectKey(SceneObjectRecord& target);
    void setFrame(std::uint32_t frame, const SceneObjectAnimationServices& services);
    void move(const SceneObjectAnimationServices& services);
    void cloak(SceneObjectCollection& objects, const SceneObjectAnimationServices& services);
    void uncloak(SceneObjectCollection& objects, const SceneObjectAnimationServices& services);
    void initializeFromEntity(int index, ssgEntity* source, const SceneObjectAnimationServices& services);
};
ssgLeaf* findFirstLeaf(ssgEntity* entity);
bool containsSceneEntity(ssgEntity* entity, ssgEntity* target);
void extendSceneBoundsFromVertices(ssgEntity* entity, sgBox& bounds);
void setSceneRenderOrderMode(ssgEntity* entity, int mode);
bool sceneHasTransparency(ssgEntity* entity);
void updateSceneTransparency(ssgEntity* entity, float alpha);
void accumulateEntityTransform(ssgEntity* entity, std::uint32_t frame, sgMat4 matrix);
void accumulateAncestorTransforms(ssgEntity* stop, ssgEntity* entity, std::uint32_t frame, sgMat4 matrix);
struct SceneObjectCollection {
    void setDefaultSound(const char*);
    ssgEntity* sceneRoot;
    SceneObjectRecord* objects;
    int objectCount;
    SceneTriangleRecord* triangleStorage;
    int triangleCount;
    SceneObjectRecord* sentinelObject;
    SceneObjectRecord** sortedNamedObjects;
    int namedObjectCount;
    SceneTriangleServices triangleServices();
    void propagateCloak(ssgEntity* entity, bool inherited);
    SceneObjectRecord* findWithinRoot(ssgEntity* root, const char* name);
    SceneObjectRecord* findByName(const char* name) const;
    void resetAnimationFrames(const SceneObjectAnimationServices& services);
    void countSceneRecords(ssgEntity* entity, bool renderOnly);
    void collectSceneRecords(ssgEntity* entity, int objectIndex, std::uint32_t frames,
                             bool renderOnly, const SceneObjectAnimationServices& services);
    void buildFromScene(ssgEntity* root, const SceneObjectAnimationServices& services);
    void releaseSceneRecords();
};
}
