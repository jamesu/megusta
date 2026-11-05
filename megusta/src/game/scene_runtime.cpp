#include "scene_runtime.h"
#include "scene_object_literals.h"
#include "ssg.h"
#include "ssgGameAnimation.h"
#include "engine/platform_services.h"
#include "engine/resource_runtime.h"
#include "engine/string_services.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cstring>
namespace megusta {
SceneRoots gameSceneRoots;
namespace {
SceneLoadState* sceneLoading=nullptr;
constexpr char noTextureLimitPrefix[]="pup"; // 005029c0, verified through canonical Ghidra data.
}
void bindSceneLoading(SceneLoadState& state) { sceneLoading=&state; }
ssgTexture* SceneModelOptions::createTexture(char* name,int wrapU,int wrapV,int mipmap) {
    if (!enabled) return nullptr;
    return ssgLoaderOptions::createTexture(name,wrapU,wrapV,mipmap);
}
ssgEntity* loadSceneModel(const char* name,bool textureEnabled,SceneModelOptions* options) {
    if (!sceneLoading)
        throw std::logic_error("Scene loading requires recovered load policy");
    auto& runtime=*sceneLoading;
    const int savedLimit=runtime.textureLimit;
    if (!strings::compareInsensitiveBounded(name,noTextureLimitPrefix,3)) runtime.textureLimit=0;
    auto* previous=ssgGetCurrentOptions();
    SceneModelOptions defaultOptions;
    if (!options) options=&defaultOptions;
    options->enabled=textureEnabled;
    ssgSetCurrentOptions(options);
    char path[128]; ssgEntity* result=nullptr;
    switch (runtime.method) {
    case 0: std::sprintf(path,"%s.ase",name); break;
    case 1: std::sprintf(path,"%s.asx",name); break;
    case 2: {
        std::sprintf(path,"%s/%s.asx",options->getModelDir(),name);
        if (std::strlen(path)>127) std::abort();
        ResourceStatus status{};
        const int exists=sharedIo?sharedIo->queryStat(path,status):-1;
        std::sprintf(path,exists==0?"%s.asx":"%s.ase",name); break;
    }
    case 3: std::sprintf(path,"%s.ps2.asx",name); break;
    default: if (previous) ssgSetCurrentOptions(previous); runtime.textureLimit=savedLimit; std::abort();
    }
    if (std::strlen(path)>127) std::abort();
    result=ssgLoad(path,options);
    if (previous) ssgSetCurrentOptions(previous);
    runtime.textureLimit=savedLimit;
    if (!result) std::abort();
    return result;
}
ssgTransform* createSceneTransform(float x,float y,float z,float heading) {
    sgVec3 axis={0,0,1}; sgMat4 rotation,translation;
    sgMakeRotMat4(rotation,heading,axis);
    sgMakeTransMat4(translation,x,y,z);
    sgPostMultMat4(rotation,translation);
    auto* result=new ssgTransform;
    result->setTransform(rotation);
    return result;
}
ssgAnim* createSceneAnimation(ssgEntity* entity) { return new ssgAnim(entity); }
void drawSceneRoot() { ssgCullAndDraw(gameSceneRoots.root); }
void setSceneCamera(const float* eye,const float* target) {
    sgVec3 position={eye[0],eye[1],eye[2]},lookAt={target[0],target[1],target[2]};
    ssgSetCameraLookAt(position,lookAt);
    ssgSetFOV(60.0f,0.0f);
}
void SceneRoots::initialize() { root=new ssgRoot; }
void SceneRoots::initializeSecondary() { secondaryRoot=new ssgRoot; }
void SceneRoots::clear() {
    root->removeAllKids();
    if (root->getNumKids()==0) { delete root; root=new ssgRoot; }
    if (secondaryRoot) { delete secondaryRoot; secondaryRoot=nullptr; }
}
void hideSceneEntity(ssgEntity* entity) { entity->clrTraversalMaskBits(1); }
void showSceneEntity(ssgEntity* entity) { entity->setTraversalMaskBits(1); }
void hideFakeSceneObjects(ssgEntity* entity) {
    const char* name=entity->getName();
    if (name && (!std::strncmp(name,scene_object_literals::hiddenPrefixHp,3) ||
                 !std::strncmp(name,scene_object_literals::hiddenPrefixCo,3))) hideSceneEntity(entity);
    if (entity->isAKindOf(ssgTypeBranch())) {
        auto* branch=static_cast<ssgBranch*>(entity);
        for (int i=0;i<branch->getNumKids();++i) hideFakeSceneObjects(branch->getKid(i));
    }
}
}
