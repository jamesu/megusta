#pragma once
#include "scene_triangle.h"
#include "../engine/runtime_adapters.h"

#if defined(_MSC_VER)
#define MEGUSTA_GAME_FASTCALL __fastcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define MEGUSTA_GAME_FASTCALL __attribute__((fastcall))
#else
#define MEGUSTA_GAME_FASTCALL
#endif

namespace megusta::game_vectors {
Point3F* MEGUSTA_RUNTIME_THISCALL Point3_subtractInPlace_impl(Point3F* point,const Point3F* other);
float MEGUSTA_GAME_FASTCALL Point3_length_impl(Point3F* point);
void Point3F_normalize_impl(float* vector);
long double Point3F_dot_impl(const float* first,const float* second);
long double Point3F_dist_impl(const float* first,const float* second);
void Point3F_add_impl(float* first,const float* second);
void Point3F_sub_impl(float* first,const float* second);
void Point3F_scale_impl(float* vector,float length);
long double Point3F_getAngle_impl(const float* vector);
void Point3F_setPolar_impl(float* vector,float radius,float angleDegrees);
Point3F* gameMath_setVec3_impl(Point3F* point,float x,float y,float z);
void gameMath_scaleVec3_impl(Point3F* point,float scale);
}
