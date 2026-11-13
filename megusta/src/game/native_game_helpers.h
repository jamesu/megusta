// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "ssg.h"
#include "pu.h"
class fntRenderer;
namespace megusta {
void rendererSetColor(fntRenderer*,float,float,float,float);
void rendererShadowPuts(fntRenderer*,const char*,int,int,float,float,float,float);
void copyGuiDefaultFont(puFont*);
void setTransformPosition(ssgBaseTransform*,const float*);
void transformShapeDirection(float*,const float*,const float*);
void makeShapeAxisAngleMatrix(float*,float,float,float,float);
extern float gameNodeAimVector[3];
void setNodeAimVector(float,float,float);
void setLeafDrawCallbackRecursive(ssgEntity*,ssgCallback);
void bindEditingState(int*);
bool isEditing();
void bindDemoState(bool*);
bool isDemoMode();
void reportGameDiagnostic(const char*,int,const char*,bool);
void launchConfigurationTool(const char*);
struct WireDebugState {
    int mode=0;
    int selectedObjectIndex=1,selectedTriangleIndex=0;
    void initialize();
    void collectBallCandidates(class SceneGridIndex&,struct SceneObjectCollection&,const class Ball&);
    void applySceneCallbacks(struct SceneObjectCollection&,class SceneGridIndex&,
        ssgCallback allCallback,ssgCallback triangleCallback,ssgCallback candidatesCallback);
};
}
