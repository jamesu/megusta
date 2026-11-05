#pragma once
#include "scene_triangle.h"
namespace megusta {
int intersectRaySphere(const Point3F& start, const Point3F& end, const Point3F& center,
                       float radius, Point3F& first, Point3F& second);
bool clipParametricInequality(float coefficient, float distance, float& begin, float& end);
bool clipSegmentBoxSlabs(const Point3F& origin, const Point3F& direction,
                         const Point3F& halfExtents, float& begin, float& end);
int intersectSegmentOrientedBox(const Point3F& start, const Point3F& end,
    const Point3F& center, const Point3F& halfExtents, const sgMat4 rotation,
    Point3F& first, Point3F& second);
}
