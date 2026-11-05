#include "native_game_helpers.h"
#include "scene_grid.h"
#include "ball.h"
#include "engine/engine_math.h"
#include "engine/resource_runtime.h"
#include "engine/runtime_adapters.h"
#include "task_constants.h"
#include "fnt.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
namespace { int initialEditing=0; int* editing=&initialEditing; bool initialDemo=false; bool* demo=&initialDemo; }
float gameNodeAimVector[3]{};
void rendererSetColor(fntRenderer*,float r,float g,float b,float a) { glColor4f(r,g,b,a); }
void rendererShadowPuts(fntRenderer* renderer,const char* text,int x,int y,float r,float g,float b,float a) {
    renderer->start2f(static_cast<float>(x),static_cast<float>(y)); glColor4f(0,0,0,a); renderer->puts(text);
    renderer->start2f(static_cast<float>(x-1),static_cast<float>(y+1)); glColor4f(r,g,b,a); renderer->puts(text);
}
void copyGuiDefaultFont(puFont* output) { *output=puGetDefaultLegendFont(); }
void setTransformPosition(ssgBaseTransform* transform,const float* position) {
    sgMat4 matrix; transform->getTransform(matrix);
    matrix[3][0]=position[0]; matrix[3][1]=position[1]; matrix[3][2]=position[2]; transform->setTransform(matrix);
}
void transformShapeDirection(float* output,const float* direction,const float* matrix) {
    const float x=direction[0],y=direction[1],z=direction[2];
    output[0]=x*matrix[0]+y*matrix[1]+z*matrix[2];
    output[1]=x*matrix[4]+y*matrix[5]+z*matrix[6];
    output[2]=x*matrix[8]+y*matrix[9]+z*matrix[10];
}
void makeShapeAxisAngleMatrix(float* output,float angle,float x,float y,float z) {
    const float sine=static_cast<float>(engine_math::engineMath_sin_impl(angle));
    const float cosine=static_cast<float>(engine_math::engineMath_cos_impl(angle));
    const float remainder=1.0f-cosine,xy=y*x*remainder,xz=z*x*remainder,yz=y*z*remainder;
    output[0]=x*x*remainder+cosine; output[1]=z*sine+xy; output[2]=xz-y*sine; output[3]=0;
    output[4]=xy-z*sine; output[5]=y*y*remainder+cosine; output[6]=x*sine+yz; output[7]=0;
    output[8]=y*sine+xz; output[9]=yz-x*sine; output[10]=z*z*remainder+cosine; output[11]=0;
    output[12]=0; output[13]=0; output[14]=0; output[15]=1;
}
void setNodeAimVector(float x,float y,float z) { gameNodeAimVector[0]=x; gameNodeAimVector[1]=y; gameNodeAimVector[2]=z; }
void setLeafDrawCallbackRecursive(ssgEntity* entity,ssgCallback callback) {
    if (entity->isAKindOf(ssgTypeBranch())) {
        auto* branch=static_cast<ssgBranch*>(entity);
        for (int i=0;i<branch->getNumKids();++i) setLeafDrawCallbackRecursive(branch->getKid(i),callback);
    }
    if (entity->isAKindOf(ssgTypeLeaf())) static_cast<ssgLeaf*>(entity)->setCallback(SSG_CALLBACK_PREDRAW,callback);
}
void bindEditingState(int* state) { editing=state?state:&initialEditing; }
bool isEditing() { return *editing!=0; }
void bindDemoState(bool* state) { demo=state?state:&initialDemo; }
bool isDemoMode() { return *demo; }
void reportGameDiagnostic(const char* file,int line,const char* message,bool fatal) {
    sharedIo->standardError->printf(fatal?task_constants::fatalFormat:"WARNING: %s @ %d: %s\n",file,line,message);
    if (fatal) std::exit(1);
}
void launchConfigurationTool(const char* option) {
    const char* module=engine_runtime::currentExecutablePath();
    char tool[260],restart[260]; std::strcpy(tool,module);
    char* separator=std::strrchr(tool,'\\'); if (!separator) separator=std::strrchr(tool,'/');
    std::strcpy(separator?separator+1:tool,"MinigolfConfig.exe"); std::sprintf(restart,"-run:%s -nointro",module);
    const char* arguments[4]={tool,restart,nullptr,nullptr};
    if (option) { arguments[1]=option; arguments[2]=restart; }
    engine_runtime::engineRuntime_openUrlOrExec_impl(tool,arguments);
}
void WireDebugState::initialize() { mode=3; selectedObjectIndex=1; selectedTriangleIndex=0; }
void WireDebugState::collectBallCandidates(SceneGridIndex& grid,SceneObjectCollection& scene,const Ball& ball) {
    if (mode==6) { const Point3F position{ball.position[0],ball.position[1],ball.position[2]}; grid.collectSegmentCandidates(scene,position,position); }
}
void WireDebugState::applySceneCallbacks(SceneObjectCollection& scene,SceneGridIndex& grid,
        ssgCallback allCallback,ssgCallback triangleCallback,ssgCallback candidatesCallback) {
    setLeafDrawCallbackRecursive(scene.sceneRoot,nullptr);
    switch (mode) {
    case 1: setLeafDrawCallbackRecursive(scene.objects[selectedObjectIndex].entity,allCallback); break;
    case 2: setLeafDrawCallbackRecursive(scene.objects[selectedObjectIndex].entity,triangleCallback); break;
    case 3:
        for (int i=1;i<=scene.objectCount;++i) if ((scene.objects[i].flags&2)==2)
            setLeafDrawCallbackRecursive(scene.objects[i].entity,allCallback);
        break;
    case 4: setLeafDrawCallbackRecursive(scene.sceneRoot,allCallback); break;
    case 5:
        for (int i=1;i<=scene.objectCount;++i) if (!(scene.objects[i].flags&1))
            setLeafDrawCallbackRecursive(scene.objects[i].entity,allCallback);
        break;
    case 6: setLeafDrawCallbackRecursive(scene.sceneRoot,candidatesCallback); grid.candidateCount=0; break;
    }
}
}
