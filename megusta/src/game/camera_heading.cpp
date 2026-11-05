#include "camera.h"
#include "recovered_binding_primitives.h"
#include <cmath>
namespace megusta {
float Camera::headingFromOrbitVector(const Point3F& direction) {
    const long double length=binding_math_sqrtf(
        direction.x*direction.x+direction.y*direction.y+direction.z*direction.z);
    float normalizedX=direction.x;
    if (length!=0.0L) normalizedX=static_cast<float>(static_cast<long double>(direction.x)/length);
    long double angle=std::asin(static_cast<long double>(normalizedX))*
        static_cast<long double>(57.29578f);
    if (normalizedX>0.0f) angle=180.0L-angle;
    return static_cast<float>(angle);
}
void Camera::captureOrbitHeading() {
    if (state!=CameraMode::Secure && state!=CameraMode::Sky && state!=CameraMode::Ball &&
        state!=CameraMode::Frozen && state!=CameraMode::Internal17 && state!=CameraMode::Control) return;
    const float x=controlDirection.x-controlFramePosition.x,y=controlDirection.y-controlFramePosition.y,z=controlDirection.z-controlFramePosition.z;
    if (std::fabs(x)<1e-6L && std::fabs(y)<1e-6L && std::fabs(z)<1e-6L) return;
    headingDegrees=headingFromOrbitVector({x,y,z});
}
}
