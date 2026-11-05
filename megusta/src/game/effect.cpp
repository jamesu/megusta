#include "effect.h"
#include <cmath>
#include <limits>
namespace megusta {
// Model-authored from saved bodies 0043ff60..004401d0.
void Effect::start() {
    if (emissionState==0) {
        emissionState=1;
        for (auto& particle:particles) particle.remainingTicks=0;
    }
}
void Effect::stop() { emissionState=-1; }
void Effect::setPosition(float x,float y,float z,float angle) {
    position[0]=x; position[1]=y; position[2]=z; angleDegrees=angle;
}
void Effect::setVolume(float x,float y,float z) {
    emissionVolume[0]=x; emissionVolume[1]=y; emissionVolume[2]=z;
}
void Effect::setGround(float x,float y,float z) {
    ground[0]=x; ground[1]=y; ground[2]=z;
}
void Effect::setDirection(float x,float y,float z) {
    direction[0]=x; direction[1]=y; direction[2]=z;
}
void Effect::setDispersion(float x,float y,float z) {
    dispersion[0]=x; dispersion[1]=y; dispersion[2]=z;
}
void Effect::setGravity(float x,float y,float z) {
    gravity[0]=x; gravity[1]=y; gravity[2]=z;
}
// __ftol truncates through a signed 64-bit result before taking its low word.
static std::int32_t truncateField(float value) {
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__)) && !defined(MEGUSTA_PORTABLE_NUMERIC)
    std::int64_t result;
    unsigned short original,truncate;
    asm volatile("fnstcw %0":"=m"(original));
    truncate=(original&~0x0c00u)|0x0c00u;
    asm volatile("flds %1; fldcw %2; fistpq %0; fldcw %3"
        :"=m"(result):"m"(value),"m"(truncate),"m"(original):"st");
    return static_cast<std::int32_t>(result);
#else
    // FISTP returns integer-indefinite for NaN, infinity and overflow.
    // Check the exact power-of-two boundary before converting to avoid C++ UB.
    const long double input=value;
    std::int64_t result=std::numeric_limits<std::int64_t>::min();
    if (std::isfinite(input) && input>=-0x1p63L && input<0x1p63L)
        result=static_cast<std::int64_t>(input);
    const std::uint32_t low=static_cast<std::uint32_t>(result);
    return low<=0x7fffffffu ? static_cast<std::int32_t>(low)
        : static_cast<std::int32_t>(static_cast<std::int64_t>(low)-0x100000000LL);
#endif
}
void Effect::setRamp(float value) { rampEnabled=truncateField(value); }
void Effect::setScale(float value) { scale[2]=value; scale[1]=value; scale[0]=value; }
void Effect::setDuration(float value) { durationTicks=truncateField(value); }
void Effect::setMinVel(float value) { minimumSpeedSquaredThreshold=value; }
void Effect::setStyle(float value) { style=truncateField(value); }
void Effect::setBlend(float value) { blendMode=truncateField(value); }
void Effect::setFriction(float value) { friction[2]=value; friction[1]=value; friction[0]=value; }
}
