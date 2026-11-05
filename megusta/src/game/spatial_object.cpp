#include "spatial_object.h"
namespace megusta {
const char* SpatialRegion::typeTag() const { return "SpatialRegion"; }
const char* SpatialTarget::typeTag() const { return "SpatialTarget"; }
const char* SpatialZone::typeTag() const { return "SpatialZone"; }
void SpatialRegion::removeLinkedId(const SpatialObject* other) {
    if (other) {
        for (int& id : linkedIds) {
            if (id==other->spatialId()) id=0;
        }
    }
}
// 00418930: perform both clamps after the addition, retaining NaN behavior.
void SpatialObject::adjustRadiusLimited(float delta) {
    radius_ = delta + radius_;
    if (radius_ < 2.0f) radius_ = 2.0f;
    if (20.0f < radius_) radius_ = 20.0f;
}
void SpatialObject::adjustRadius(float delta) { radius_ = delta + radius_; }
void SpatialObject::translate(float x, float y, float z) {
    transform_[3][0] = x + transform_[3][0];
    transform_[3][1] = y + transform_[3][1];
    transform_[3][2] = z + transform_[3][2];
}
void SpatialObject::cycleFlags() { flags_ = (flags_ + 0x100u) & 0x300u; }
SpatialCamera::SpatialCamera() : lookAt_{0.0f, 0.0f, 0.0f} {}
const char* SpatialCamera::typeTag() const { return "SpatialCamera"; }
const char* SpatialRepel::typeTag() const { return "SpatialRepel"; }
void SpatialRepel::addEulerRotation(float x, float y, float z) {
    const float px = transform_[3][0], py = transform_[3][1], pz = transform_[3][2];
    eulerAnglesDegrees_[0] = x + eulerAnglesDegrees_[0];
    eulerAnglesDegrees_[1] = y + eulerAnglesDegrees_[1];
    eulerAnglesDegrees_[2] = z + eulerAnglesDegrees_[2];
    sgVec3 axis = {0.0f, 0.0f, 1.0f};
    sgMakeRotMat4(transform_, eulerAnglesDegrees_[0], axis);
    transform_[3][0] = px; transform_[3][1] = py; transform_[3][2] = pz;
}
}

#include "spatial_literals.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
SpatialRepel::~SpatialRepel() { std::free(heightGrid_); }
int SpatialCamera::writeRecord(char* text, std::uint32_t) const {
    std::strcpy(text, typeTag());
    std::sprintf(text + std::strlen(typeTag()), spatial_literals::cameraWrite,
        spatialId_, static_cast<double>(transform_[3][0]), static_cast<double>(transform_[3][1]),
        static_cast<double>(transform_[3][2]), static_cast<double>(radius_), flags_,
        static_cast<double>(lookAt_[0]), static_cast<double>(lookAt_[1]), static_cast<double>(lookAt_[2]));
    return static_cast<int>(std::strlen(text));
}
int SpatialCamera::parseRecord(const char* text, std::uint32_t) {
    float x, y, z;
    std::sscanf(text + std::strlen(typeTag()), spatial_literals::readRecord,
        &spatialId_, &x, &y, &z, &radius_, reinterpret_cast<int*>(&flags_),
        &lookAt_[0], &lookAt_[1], &lookAt_[2]);
    sgMakeIdentMat4(transform_);
    transform_[3][0] = x; transform_[3][1] = y; transform_[3][2] = z;
    return 1;
}
int SpatialRepel::writeRecord(char* text, std::uint32_t) const {
    std::strcpy(text, typeTag());
    std::sprintf(text + std::strlen(typeTag()), spatial_literals::repelWrite,
        spatialId_, static_cast<double>(transform_[3][0]), static_cast<double>(transform_[3][1]),
        static_cast<double>(transform_[3][2]), static_cast<double>(radius_), flags_,
        static_cast<double>(eulerAnglesDegrees_[0]), static_cast<double>(eulerAnglesDegrees_[1]),
        static_cast<double>(eulerAnglesDegrees_[2]));
    return static_cast<int>(std::strlen(text));
}
int SpatialRepel::parseRecord(const char* text, std::uint32_t) {
    float x, y, z;
    std::sscanf(text + std::strlen(typeTag()), spatial_literals::readRecord,
        &spatialId_, &x, &y, &z, &radius_, reinterpret_cast<int*>(&flags_),
        &eulerAnglesDegrees_[0], &eulerAnglesDegrees_[1], &eulerAnglesDegrees_[2]);
    sgMakeIdentMat4(transform_);
    transform_[3][0] = x; transform_[3][1] = y; transform_[3][2] = z;
    addEulerRotation(0.0f, 0.0f, 0.0f);
    return 1;
}
void SpatialRepel::cycleConstraintKind(void (*showMessage)(const char*)) {
    flags_ = (flags_ + 0x100u) & 0x300u;
    switch (flags_) {
    case 0: showMessage("Sphere"); break;
    case 0x100: showMessage("Flat plane"); break;
    case 0x200: showMessage(spatial_literals::boxLabel); break;
    case 0x300: showMessage("Dropcloth"); break;
    }
}
void SpatialCamera::cycleMode(void (*showMessage)(const char*)) {
    flags_ += 0x100u;
    if (static_cast<std::int32_t>(flags_) > 0x700) flags_ &= 0xffu;
    switch (flags_) {
    case 0: showMessage("Security"); break;
    case 0x100: showMessage("Frozen"); break;
    case 0x200: showMessage("Cinematic"); break;
    case 0x300: showMessage("Puttoff"); break;
    case 0x400: showMessage("Bird Eye"); break;
    case 0x500: showMessage("Bird Target"); break;
    case 0x600: showMessage("Chase Cam"); break;
    case 0x700: showMessage("Just Do It Cam"); break;
    }
}
}
namespace megusta {
// 00434ad0: sample an 8-by-8 world-space footprint; out-of-grid samples
// use the original -999 sentinel. Wide intermediates preserve x87 iteration.
bool SpatialRepel::testPointHeightClearance(const float* point) const {
    const long double xEnd = static_cast<long double>(point[0]) + 4.0L;
    const long double yEnd = static_cast<long double>(point[1]) + 4.0L;
    for (long double x = static_cast<long double>(point[0]) - 4.0L; x < xEnd; x += 1.0L) {
        const float gx = static_cast<float>((x - gridMinX_) * gridScaleX_);
        for (long double y = static_cast<long double>(point[1]) - 4.0L; y < yEnd; y += 1.0L) {
            const float gy = static_cast<float>((y - gridMinY_) * gridScaleY_);
            float height = -999.0f;
            if (!(gx < 0.0f || gy < 0.0f || static_cast<float>(gridColumns_) <= gx || static_cast<float>(gridRows_) <= gy))
                height = heightGrid_[static_cast<int>(gy) * gridColumns_ + static_cast<int>(gx)];
            if (static_cast<long double>(point[2]) - 4.0L < height) {
                const float dy2 = static_cast<float>((point[1] - y) * (point[1] - y));
                const float dx2 = static_cast<float>((point[0] - x) * (point[0] - x));
                if (static_cast<long double>(dx2) + dy2 <= 16.0L &&
                    (point[2] <= height ||
                     (static_cast<long double>(point[2]) - height) * (static_cast<long double>(point[2]) - height) + dx2 + dy2 <= 16.0L))
                    return true;
            }
        }
    }
    return false;
}
}

#include "scene_triangle.h"
#include <cmath>
namespace megusta {
// 00434890: rebuild before the footprint test; clamp shared camera fields,
// which may alias pitch/distance, before deriving the next orbit position.
bool SpatialRepel::adjustOrbitAgainstHeightGrid(const Point3F& eye, const Point3F&,
    const Point3F& target, Point3F& eyeOut, float& pitch, float& distance,
    const float& heading, std::uint32_t, const CameraConstraintServices& services) {
    const Point3F initialEye=eye, targetCopy=target;
    const float centerX=transform_[3][0], centerY=transform_[3][1];
    services.rebuildHeightGrid(services.context,*this);
    if (initialEye.x+4.0f<centerX-radius_ || initialEye.y+4.0f<centerY-radius_ ||
        centerX+radius_<initialEye.x-4.0f || centerY+radius_<initialEye.y-4.0f) return false;
    float sample[3]={initialEye.x,initialEye.y,initialEye.z};
    bool moved=false;
    for (int i=0;i<99;++i) {
        if (!testPointHeightClearance(sample)) return moved;
        pitch=pitch+1.0f;
        distance=distance-2.0f;
        if (5.0f<services.globalPitchDegrees) {
            if (80.0f<services.globalPitchDegrees) services.globalPitchDegrees=80.0f;
        } else services.globalPitchDegrees=5.0f;
        if (10.0f<=services.globalFollowDistance) {
            if (!services.isEditing(services.context) && 130.0f<services.globalFollowDistance)
                services.globalFollowDistance=130.0f;
        } else services.globalFollowDistance=10.0f;
        const float pitchAngle=static_cast<float>(pitch*0.017453292);
        const float horizontal=static_cast<float>(std::cos(static_cast<long double>(pitchAngle))*distance);
        const float headingAngle=static_cast<float>(heading*-0.017453292);
        const float dx=static_cast<float>(std::cos(static_cast<long double>(headingAngle))*horizontal);
        const float dy=static_cast<float>(std::sin(static_cast<long double>(headingAngle))*horizontal);
        const long double vertical=std::sin(static_cast<long double>(pitchAngle));
        sample[0]=targetCopy.x+dx; sample[1]=targetCopy.y+dy;
        eyeOut.x=sample[0]; eyeOut.y=sample[1];
        sample[2]=static_cast<float>(static_cast<long double>(targetCopy.z)+vertical*distance);
        eyeOut.z=sample[2]; moved=true;
        if (80.0f<=pitch || distance<=10.0f) return true;
    }
    return true;
}
}
