#pragma once
#include "scene_triangle.h"
#include <cstdint>

namespace megusta {
Point3F* binding_copy_point3(Point3F* point,const Point3F* other);
std::uint16_t PowerUps_collide_bounds_impl(float centerX,float centerY,float centerZ,
    float boundX,float boundY,float boundZ,float radius);
float binding_math_sqrtf(float value);
char* binding_string_copy(char* output,const char* source);
}
