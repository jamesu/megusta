#include "vector_primitives.h"
#include "../engine/engine_math.h"
#include "recovered_binding_primitives.h"
#include <cmath>

namespace megusta::game_vectors {

Point3F* MEGUSTA_RUNTIME_THISCALL Point3_subtractInPlace_impl(Point3F* point,const Point3F* other) {
    point->x-=other->x; point->y-=other->y; point->z-=other->z;
    return point;
}
float MEGUSTA_GAME_FASTCALL Point3_length_impl(Point3F* point) {
    return binding_math_sqrtf(point->z*point->z+point->y*point->y+point->x*point->x);
}
void Point3F_normalize_impl(float* vector) {
    const long double length=binding_math_sqrtf(
        vector[2]*vector[2]+vector[1]*vector[1]+vector[0]*vector[0]);
    if(length!=0.0L) {
        const long double inverse=1.0L/length;
        vector[0]=static_cast<float>(inverse*vector[0]);
        vector[1]=static_cast<float>(inverse*vector[1]);
        vector[2]=static_cast<float>(inverse*vector[2]);
    }
}
long double Point3F_dot_impl(const float* first,const float* second) {
    return static_cast<long double>(first[0])*second[0]+
        static_cast<long double>(first[1])*second[1]+
        static_cast<long double>(first[2])*second[2];
}
long double Point3F_dist_impl(const float* first,const float* second) {
    // The recovered routine passes the squared distance to the float-valued
    // binding_math_sqrtf helper. Preserve its float argument boundary and the
    // original x, z, y accumulation order instead of keeping the whole sum in
    // long double.
    return binding_math_sqrtf(
        (first[0]-second[0])*(first[0]-second[0])+
        (first[2]-second[2])*(first[2]-second[2])+
        (first[1]-second[1])*(first[1]-second[1]));
}
void Point3F_add_impl(float* first,const float* second) {
    first[0]+=second[0]; first[1]+=second[1]; first[2]+=second[2];
}
void Point3F_sub_impl(float* first,const float* second) {
    first[0]-=second[0]; first[1]-=second[1]; first[2]-=second[2];
}
void Point3F_scale_impl(float* vector,float length) {
    const long double current=binding_math_sqrtf(
        vector[2]*vector[2]+vector[1]*vector[1]+vector[0]*vector[0]);
    if(current!=0.0L) {
        const long double scale=static_cast<long double>(length)/current;
        vector[0]=static_cast<float>(scale*vector[0]);
        vector[1]=static_cast<float>(scale*vector[1]);
        vector[2]=static_cast<float>(scale*vector[2]);
    }
}
long double Point3F_getAngle_impl(const float* vector) {
    return engine_math::engineMath_atan2_impl(vector[1],vector[0])*
        static_cast<long double>(57.29578f);
}
void Point3F_setPolar_impl(float* vector,float radius,float angleDegrees) {
    const float radians=static_cast<float>(static_cast<long double>(angleDegrees)*
        static_cast<long double>(0.017453292f));
    vector[0]=static_cast<float>(engine_math::engineMath_cos_impl(radians)*radius);
    vector[1]=static_cast<float>(engine_math::engineMath_sin_impl(radians)*radius);
}
Point3F* gameMath_setVec3_impl(Point3F* point,float x,float y,float z) {
    point->x=x; point->y=y; point->z=z;
    return point;
}
void gameMath_scaleVec3_impl(Point3F* point,float scale) {
    point->x*=scale; point->y*=scale; point->z*=scale;
}

} // namespace megusta::game_vectors
