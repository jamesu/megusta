#include "game/camera.h"
#include <cassert>
using namespace megusta;
int main() {
    Camera camera{};
    CameraRuntime runtime{};
    camera.headingDegrees=73;
    runtime.controlPathObject=reinterpret_cast<SceneObjectRecord*>(1);
    runtime.controlPointCount=7;
    camera.state=CameraMode::Control;
    camera.setState(runtime,CameraMode::Player);
    assert(camera.state==CameraMode::Control); // Saved state-lock behavior.
    assert(camera.headingDegrees==73); // Explicit orbit stub currently preserves heading.
    assert(camera.stopControlCam(runtime));
    assert(camera.state==CameraMode::NoAutomatic && runtime.controlPathObject==nullptr);
    assert(runtime.controlPointCount==0 && camera.controlCamArgument==0);
    runtime.controlPathObject=reinterpret_cast<SceneObjectRecord*>(1);
    camera.state=CameraMode::Player;
    assert(!camera.stopControlCam(runtime));
    assert(runtime.controlPathObject!=nullptr && camera.state==CameraMode::Player);
    camera.startControlCam(runtime,4.5f);
    assert(camera.state==CameraMode::Control && camera.controlCamArgument==4.5f);
    camera.stopControlCam(runtime);
    camera.startControlCam(runtime,8);
    assert(camera.controlCamArgument==0 && camera.state==CameraMode::NoAutomatic);
    camera.setPosition(runtime,25,1,2,3);
    assert(camera.state==CameraMode::Internal17 && runtime.overrideStartTick==25);
    assert(runtime.overrideDuration==60 && camera.controlFramePosition.z==3);
    camera.setLookat(runtime,30,4,5,6,0.25f);
    assert(runtime.overrideDuration==7 && camera.controlDirection.y==5);
    std::int32_t filter=0;
    Camera::disallowCollision(filter);
    assert(filter==1);
    return 0;
}
