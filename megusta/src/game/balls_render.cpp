#include "balls.h"
#include "player.h"
#include "putt_map.h"
#include "scene_runtime.h"
#include "ground_shadow.h"
#include "engine/opengl_adapter.h"
#include "ssgGameAnimation.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
namespace { BallsRenderState* renderState=nullptr; }
void bindBallsRenderState(BallsRenderState& state) { renderState=&state; }
void Ball::updateRenderModels() {
    if (!model) return;
    sgMat4 matrix; sgMakeTransMat4(matrix,position[0],position[1],position[2]);
    matrix[0][0]=rollingRotation[0]; matrix[0][1]=rollingRotation[2]; matrix[0][2]=rollingRotation[6];
    matrix[1][0]=rollingRotation[1]; matrix[1][1]=rollingRotation[3]; matrix[1][2]=rollingRotation[7];
    matrix[2][0]=rollingRotation[4]; matrix[2][1]=rollingRotation[5]; matrix[2][2]=rollingRotation[8];
    matrix[0][3]=matrix[1][3]=matrix[2][3]=0; model->setTransform(matrix);
    if (flags&10) { model->clrTraversalMaskBits(1); return; }
    int special=-1;
    if (colorModelIndex==15) special=0; else if (colorModelIndex==14) special=1; else if (colorModelIndex==16) special=2;
    if (special<0) { model->setTraversalMaskBits(1); for (auto* entity:specialModel) entity->clrTraversalMaskBits(1); return; }
    if (special!=1) model->clrTraversalMaskBits(1);
    for (int i=0;i<3;++i) if (i==special) specialModel[i]->setTraversalMaskBits(1); else specialModel[i]->clrTraversalMaskBits(1);
    if (special==1) sgMakeTransMat4(matrix,position[0],position[1],position[2]);
    static_cast<ssgBaseTransform*>(specialModel[special])->setTransform(matrix);
    auto* animation=specialAnimator[special]; unsigned frame=animation->getFrame()+1;
    if (frame>=static_cast<unsigned>(animation->getNumFrames())) frame=0;
    animation->setFrame(frame); resetTrail();
}
void BallsManager::draw(BallsRenderState& state) {
    for (unsigned i=0;i<state.players.playerCount;++i) balls[i].updateRenderModels();
}
long double Ball::nearestCup(SceneObjectCollection& objects,const PuttMap& map) const {
    float distance=100000; if (flags&0x10) return 100000.0L;
    for (int i=1;i<objects.objectCount;++i) {
        auto& object=objects.objects[i];
        if (std::strstr(object.name,"cup_0")) {
            const float* point=object.getPosition(); const float x=position[0]-point[0],y=position[1]-point[1],z=position[2]-point[2];
            const float squared=static_cast<float>(static_cast<long double>(x)*x+static_cast<long double>(z)*z+static_cast<long double>(y)*y);
            const long double candidate=std::sqrt(static_cast<long double>(squared)); if (candidate<distance) distance=static_cast<float>(candidate);
        }
    }
    return map.estimateRemainingDistance(position,distance);
}
void BallsManager::drawGroundShadows(BallsRenderState& state) {
    for (unsigned i=0;i<state.players.playerCount;++i) {
        auto& ball=balls[i]; if (ball.flags&10) continue;
        const Point3F start{static_cast<float>(ball.position[0]-20),ball.position[1],static_cast<float>(ball.position[2]-100)};
        const Point3F end{ball.position[0],ball.position[1],ball.position[2]}; BallContactResult hit;
        if (state.collision.castSweptBall(state.objects,state.grid,start,end,hit)) {
            if (shadowTexture) {
                glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,shadowTexture); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
                renderGroundShadow(state.viewMatrix,hit.position[0],hit.position[1],hit.position[2]+0.1f,-0.5f,0.5f,-0.5f,0.5f,-5,1,0.5f,true);
            }
            glEnable(GL_TEXTURE_2D);
        }
    }
}
void BallsManager::releaseHoleModels(BallsRenderState& state) {
    for (auto& ball:balls) {
        ball.model=nullptr; for (auto& slot:ball.trail) slot.initialize();
        for (auto* animation:ball.specialAnimator) if (animation) ssgDeRefDelete(animation);
        for (int i=0;i<3;++i) { ball.specialModel[i]=nullptr; ball.specialAnimator[i]=nullptr; }
    }
    state.gl.deleteLists(state.debugBallList,1);
}
void BallsManager::loadHoleModels(BallsRenderState& state) {
    // 004eab7c: four executable-resident trail opacity values.
    static constexpr float trailAlphas[4]={0.7f,0.5f,0.3f,0.1f};
    for (unsigned i=0;i<4;++i) {
        auto& ball=balls[i]; currentBall=&ball; ball.lastContactObjectIndex=0; ball.resetOffCourse();
        for (float& word:ball.rollingRotation) word=0; ball.rollingRotation[0]=ball.rollingRotation[3]=ball.rollingRotation[8]=1;
        if (ball.colorModelIndex==17) ball.setColour(i,[] { std::abort(); });
        if (i>=state.players.playerCount) continue;
        char name[32]; std::sprintf(name,"golfba%i",ball.colorModelIndex+1);
        auto* entity=loadSceneModel(name,true); auto* transform=new ssgTransform; ball.model=transform;
        transform->addKid(entity); gameSceneRoots.root->addKid(transform);
        ssgEntity* trails[4];
        for (int n=0;n<4;++n) { trails[n]=loadSceneModel(name,true); updateSceneTransparency(trails[n],trailAlphas[n]); }
        for (auto& slot:ball.trail) slot.initializeModels(trails,4,gameSceneRoots.root);
        const char* names[3]={"sheild","powerups/PUPLIT","powerups/PWRFlameBall"};
        for (int n=0;n<3;++n) { ball.specialModel[n]=nullptr; ball.specialAnimator[n]=nullptr; }
        for (int n=0;n<3;++n) {
            auto* special=loadSceneModel(names[n],true); if (!special) std::abort();
            auto* root=new ssgTransform; root->addKid(special); root->clrTraversalMaskBits(1); gameSceneRoots.root->addKid(root);
            ball.specialModel[n]=root; ball.specialAnimator[n]=createSceneAnimation(root); ball.specialAnimator[n]->ref();
        }
    }
    currentBall=nullptr; state.debugBallList=state.gl.generateLists(1); state.gl.newList(state.debugBallList,0xa3);
    glColor4f(0,0,0,0.6f); state.gl.drawSphere(0.5f,10,5); glEndList();
}
}
