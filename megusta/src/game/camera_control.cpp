#include "camera.h"
#include "player.h"
#include "ball.h"
#include "scene_object.h"
#include <cmath>
namespace megusta {
void Camera::setState(CameraRuntime& runtime,CameraMode requested) {
    runtime.orbitBlendFrame=0;
    runtime.transitionVector={0,0,0};
    if (requested==CameraMode::NoAutomatic) {
        mouseCaptured=0;
        runtime.activeRegion=nullptr;
        runtime.activeSpatialCamera=nullptr;
    }
    if (state!=CameraMode::NoAutomatic) captureOrbitHeading();
    if (state!=CameraMode::Control) state=requested;
}
void Camera::startControlCam(CameraRuntime& runtime,float argument) {
    if (runtime.controlPathObject) {
        controlCamArgument=argument;
        setState(runtime,CameraMode::Control);
        return;
    }
    controlCamArgument=0;
}
void Camera::returnToAutomatic(CameraRuntime& runtime) { setState(runtime,CameraMode::NoAutomatic); }
bool Camera::stopControlCam(CameraRuntime& runtime) {
    if (state!=CameraMode::Control && state!=CameraMode::Frozen) {
        runtime.controlPointCount=0;
        return false;
    }
    runtime.controlPointCount=0;
    runtime.controlPathObject=nullptr;
    controlCamArgument=0;
    state=CameraMode::NoAutomatic;
    setState(runtime,CameraMode::NoAutomatic);
    return true;
}
void Camera::advanceControlPath(CameraRuntime& runtime,
        const SceneObjectAnimationServices& animation,const CameraServices& services) {
    if (!runtime.controlPathObject) services.fatal();
    const auto frameCount=runtime.controlPathObject->frameCount;
    auto frame=frameCount-1;
    if (runtime.controlPathFrame<static_cast<std::int32_t>(frame))
        frame=runtime.controlPathFrame<1 ? 0u : static_cast<std::uint32_t>(runtime.controlPathFrame);
    runtime.controlPathObject->setFrame(frame,animation);
    if (runtime.controlPathFrame<static_cast<std::int32_t>(frameCount)) {
        const auto* position=runtime.controlPathObject->getPosition();
        controlFramePosition={position[0],position[1],position[2]};
        const auto* direction=runtime.controlPathObject->getDirection();
        controlDirection={direction[0],direction[1],direction[2]};
        controlDirection.x*=20.0f; controlDirection.y*=20.0f; controlDirection.z*=20.0f;
        controlDirection.x=controlFramePosition.x+controlDirection.x;
        controlDirection.y=controlFramePosition.y+controlDirection.y;
        controlDirection.z=controlFramePosition.z+controlDirection.z;
        controlEye=controlFramePosition;
        controlLookAt=controlDirection;
    }
    runtime.controlPathFrame=static_cast<std::int32_t>(static_cast<std::uint32_t>(runtime.controlPathFrame)+1u);
    if (static_cast<std::int32_t>(frameCount)<=runtime.controlPathFrame) stopControlCam(runtime);
}
void Camera::restartFollow(CameraRuntime& runtime) {
    setState(runtime,CameraMode::NoAutomatic);
    runtime.transitionVector={0,0,0};
}
void Camera::setPosition(CameraRuntime& runtime,std::uint32_t tick,float x,float y,float z) {
    runtime.overrideStartTick=tick;
    runtime.overrideDuration=60;
    setState(runtime,CameraMode::Internal17);
    controlFramePosition={x,y,z};
}
void Camera::setLookat(CameraRuntime& runtime,std::uint32_t tick,
        float x,float y,float z,float seconds) {
    runtime.overrideStartTick=tick;
    runtime.overrideDuration=static_cast<std::uint32_t>(static_cast<std::int64_t>(
        std::nearbyint(static_cast<long double>(seconds)*30.0L)));
    setState(runtime,CameraMode::Internal17);
    controlDirection={x,y,z};
}
void Camera::setLookatPlayer(CameraRuntime& runtime) { setState(runtime,CameraMode::Player); }
void Camera::setLookatBall(CameraRuntime& runtime) {
    controlDirection=runtime.ballPosition;
    restartFollow(runtime);
}
void Camera::enterPostShotFollow(CameraRuntime& runtime,Player& player,const Ball& ball) {
    runtime.regionBallPosition={ball.position[0],ball.position[1],ball.position[2]};
    runtime.spatialBlend=0;
    setState(runtime,CameraMode::Sky);
    saveHeadingToPlayer(&player);
}
void Camera::initializeHole(CameraRuntime& runtime,const CameraServices& services) {
    runtime.headingVelocity=0; runtime.pitchVelocity=0;
    runtime.headingAcceleration=0; runtime.pitchAcceleration=0;
    followDistance=70.0f; resolvedFollowDistance=70.0f;
    headingDegrees=90.0f; resolvedHeadingDegrees=90.0f;
    pitchDegrees=19.8f; resolvedPitchDegrees=19.8f;
    services.setFov(fieldOfView,0);
    runtime.controlPathObject=nullptr;
    restartFollow(runtime);
    services.preloadSound("camzoom",2);
}
void Camera::endHole(CameraRuntime& runtime,bool captureOption,const CameraServices& services) {
    runtime.followWordCe58=0;
    runtime.followEntity=nullptr;
    services.setMouseCapture(captureOption ? 16 : 15);
    mouseCaptured=0;
    stopControlCam(runtime);
    setState(runtime,CameraMode::NoAutomatic);
}
void Camera::allowCollision(std::int32_t& typeFilter,void (*showMessage)(const char*)) {
    typeFilter=static_cast<std::int32_t>(static_cast<std::uint32_t>(typeFilter)+1u);
    if (typeFilter>5) typeFilter=0;
    switch (typeFilter) {
    case 1: showMessage("No camera collision"); return;
    case 2: showMessage("Only show dropcloths"); return;
    case 3: showMessage("Only show AI"); return;
    case 4: showMessage("Only show Powerups"); return;
    case 5: showMessage("Only show Cameras"); return;
    default: showMessage("Default Collision"); return;
    }
}
void Camera::disallowCollision(std::int32_t& typeFilter) { typeFilter=1; }
}
