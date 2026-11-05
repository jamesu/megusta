#include "recovered_binding_primitives.h"
#include <cmath>
#include <cstring>

namespace megusta {
Point3F* binding_copy_point3(Point3F* point,const Point3F* other) {
    point->x=other->x; point->y=other->y; point->z=other->z;
    return point;
}

std::uint16_t PowerUps_collide_bounds_impl(float x,float y,float z,
        float boundX,float boundY,float boundZ,float radius) {
    return z+radius>=boundZ && z-radius<=boundZ &&
           x+radius>=boundX && x-radius<=boundX &&
           y+radius>=boundY && y-radius<=boundY ? 1 : 0;
}

float binding_math_sqrtf(float value) { return std::sqrt(value); }
char* binding_string_copy(char* output,const char* source) { return std::strcpy(output,source); }
} // namespace megusta
