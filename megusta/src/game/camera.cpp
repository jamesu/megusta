#include "camera.h"
#include "engine/engine_math.h"
#include "player.h"
#include "ball.h"
#include "balls.h"
#include "putter.h"
#include <cmath>
#include "scene_object.h"
namespace megusta {
Point3F Camera::getFramingTarget(bool editing,bool offsetEnabled) const {
    if (!editing && offsetEnabled && state!=CameraMode::Control && state!=CameraMode::Internal13) {
        const float angle=headingDegrees*-0.017453292f;
        return {static_cast<float>(controlLookAt.x-engine_math::engineMath_cos_impl(angle)*followDistance*0.14285715L),
                static_cast<float>(controlLookAt.y-engine_math::engineMath_sin_impl(angle)*followDistance*0.14285715L),
                static_cast<float>(engine_math::engineMath_cos_impl(pitchDegrees*0.017453292f)*4.0L+controlLookAt.z)};
    }
    return controlLookAt;
}
bool Camera::hasFollowBallMotion(CameraRuntime& runtime,Ball& ball) {
    const auto capture=[&]() {
        runtime.followMotionBall=&ball;
        runtime.previousFollowMotionPosition={ball.position[0],ball.position[1],ball.position[2]};
    };
    if (runtime.followMotionBall==&ball &&
        (!(std::fabs(runtime.previousFollowMotionPosition.x-ball.position[0])<1e-6f) ||
         !(std::fabs(runtime.previousFollowMotionPosition.y-ball.position[1])<1e-6f) ||
         !(std::fabs(runtime.previousFollowMotionPosition.z-ball.position[2])<1e-6f))) { capture(); return true; }
    if ((ball.flags&0x2bu) && std::fabs(ball.velocity[0])<1e-6f && std::fabs(ball.velocity[1])<1e-6f && std::fabs(ball.velocity[2])<1e-6f) {
        capture(); return false;
    }
    capture(); return true;
}
bool Camera::canFollowMovingBall(CameraRuntime& runtime,bool sceneActive,const Player* player,BallsManager& balls,const Putter& putter) {
    return sceneActive && player && hasFollowBallMotion(runtime,*player->getBall(balls)) && putter.state!=1;
}
void Camera::setControlPathStep(CameraRuntime& runtime,std::int32_t step) {
    runtime.controlPathStep=step;
}
bool Camera::loadControlPath(CameraRuntime& runtime,SceneObjectCollection& objects,
        const char* name,const SceneObjectAnimationServices& animation) {
    auto* object=objects.findByName(name);
    if (object==objects.sentinelObject) return false;
    runtime.controlPathFrame=static_cast<std::int32_t>(
        0u-static_cast<std::uint32_t>(runtime.controlPathStep));
    runtime.controlPathObject=object;
    object->setFrame(0,animation);
    return true;
}
void Camera::addControlPoint(CameraRuntime& runtime,float x,float y,float z) {
    if (runtime.controlPointCount<32) {
        const int index=runtime.controlPointCount++;
        runtime.controlPoints[index].x=x;
        runtime.controlPoints[index].y=y;
        runtime.controlPoints[index].z=z;
    }
}
void Camera::addControlPoint(CameraRuntime& runtime,const Point3F& point) {
    if (runtime.controlPointCount<32) {
        const int index=runtime.controlPointCount++;
        runtime.controlPoints[index].x=point.x;
        runtime.controlPoints[index].y=point.y;
        runtime.controlPoints[index].z=point.z;
    }
}
bool Camera::checkControlCam() const {
    return state==CameraMode::Control || state==CameraMode::Frozen;
}
void Camera::targetCharacterHeight(CameraRuntime& runtime,const Player& player) {
    runtime.ballTargetOffset=0;
    float height=12.0f;
    if (followDistance<50.0f) height=(50.0f-followDistance)*0.25f+12.0f;
    const int tag=static_cast<signed char>(player.modelTag[1])*256+
                  static_cast<signed char>(player.modelTag[0]);
    switch (tag) {
    case 0x4745: case 0x5552: height+=6.0f; break;
    case 0x4c53: height+=3.0f; break;
    case 0x4843: case 0x4944: case 0x5550: case 0x4457: height-=2.0f; break;
    case 0x4148: height+=4.0f; break;
    case 0x4e49: height-=6.0f; break;
    case 0x5249: height+=8.0f; break;
    case 0x5a48: height+=2.0f; break;
    default: break;
    }
    controlDirection.x=player.characterTransform[3][0];
    controlDirection.y=player.characterTransform[3][1];
    controlDirection.z=player.characterTransform[3][2];
    controlDirection.z=height+controlDirection.z;
}
void Camera::targetBallPosition(CameraRuntime& runtime) {
    runtime.ballTargetOffset=0;
    controlDirection=runtime.ballPosition;
}
void Camera::targetBallWithOffset(CameraRuntime& runtime) {
    targetBallPosition(runtime);
    runtime.ballTargetOffset=1;
}
void transformPackedCameraVector(const Point3F& input,const float* basis,Point3F& output) {
    output.x=input.x*basis[0]+basis[6]*input.z+basis[2]*input.y;
    output.y=basis[7]*input.z+basis[3]*input.y+basis[1]*input.x;
    output.z=basis[8]*input.z+basis[5]*input.y+basis[4]*input.x;
}
void Camera::worldToViewVector(const Point3F& input,Point3F& output) const {
    const Point3F relative{static_cast<float>(input.x-controlEye.x),
        static_cast<float>(input.y-controlEye.y),static_cast<float>(input.z-controlEye.z)};
    transformPackedCameraVector(relative,viewBasis,output);
}
bool Camera::projectPointToScreen(const Point3F& input,CameraScreenPoint& output) const {
    Point3F view;
    worldToViewVector(input,view);
    if (!(view.y>0.0f)) return false;
    const float vertical=(view.z*screenProjection[1])/view.y;
    const auto horizontal=static_cast<std::int32_t>(static_cast<std::int64_t>(
        (static_cast<long double>(view.x)*screenProjection[0])/view.y));
    output.x=static_cast<std::int32_t>(screenProjection[2])+horizontal;
    const auto verticalInteger=static_cast<std::int32_t>(static_cast<std::int64_t>(vertical));
    long double depth=static_cast<long double>(view.y)*16.0L;
    output.y=static_cast<std::int32_t>(screenProjection[3])-verticalInteger;
    if (depth<65535.0L) {
        if (depth<=0.0L) depth=0.0L;
    } else depth=65535.0L;
    output.depth=static_cast<std::uint32_t>(static_cast<std::int64_t>(depth))&0xffffu;
    return true;
}
void Camera::smoothHeading(float target) {
    target=(360.0f-headingDegrees)+target;
    while (target<0.0f) target+=360.0f;
    while (target>=360.0f) target-=360.0f;
    if (target>180.0f) target-=360.0f;
    headingDegrees=target*0.1+headingDegrees;
}
bool Camera::restoreSavedHeadingAtRest(const Ball& ball) {
    if (ball.restHistoryIndex!=0) return false;
    headingDegrees=savedHeadingDegrees;
    return true;
}
void Camera::restorePlayerHeading(CameraRuntime& runtime,Player* player) {
    if (player) {
        player->controlOption94=player->controlOption98;
        runtime.headingRestoreRequested=true;
        runtime.headingAvailable=true;
    }
}
void Camera::saveHeadingToPlayer(Player* player) const {
    if (player) player->controlOption98=static_cast<std::uint32_t>(
        static_cast<std::int64_t>(headingDegrees));
}
void Camera::resetFovDefault() { fieldOfView=54.432f; }
void Camera::jitter(CameraRuntime& runtime,std::uint32_t fixedTick,float seconds) {
    runtime.jitterStartTick=fixedTick;
    runtime.jitterDuration=static_cast<std::int32_t>(
        static_cast<std::int64_t>(static_cast<long double>(seconds)*30.0L));
}
bool Camera::collisionEnabled() { return false; }
void Camera::clampEditorOrbit(bool (*isEditing)()) {
    if (pitchDegrees>5.0f) {
        if (pitchDegrees>80.0f) pitchDegrees=80.0f;
    } else pitchDegrees=5.0f;
    if (followDistance<10.0f) { followDistance=10.0f; return; }
    if (!isEditing() && followDistance>130.0f) followDistance=130.0f;
}
}
