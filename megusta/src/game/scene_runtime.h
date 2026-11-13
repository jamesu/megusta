// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "ssg.h"
class ssgRoot;
class ssgEntity;
class ssgTransform;
class ssgAnim;
namespace megusta {
class SceneModelOptions : public ssgLoaderOptions {
public:
    bool enabled=true;
    ssgTexture* createTexture(char* name,int wrapU=1,int wrapV=1,int mipmap=1) override;
};
struct SceneLoadState {
    int& method;
    int& textureLimit;
};
void bindSceneLoading(SceneLoadState&);
ssgEntity* loadSceneModel(const char*,bool,SceneModelOptions* options=nullptr);
struct SceneRoots {
    ssgRoot* root=nullptr;
    ssgRoot* secondaryRoot=nullptr;
    void initialize();
    void initializeSecondary();
    void clear();
};
extern SceneRoots gameSceneRoots;
ssgTransform* createSceneTransform(float x,float y,float z,float heading);
ssgAnim* createSceneAnimation(ssgEntity* entity);
void drawSceneRoot();
void setSceneCamera(const float* eye,const float* target);
void hideFakeSceneObjects(ssgEntity* entity);
void hideSceneEntity(ssgEntity* entity);
void showSceneEntity(ssgEntity* entity);
}
