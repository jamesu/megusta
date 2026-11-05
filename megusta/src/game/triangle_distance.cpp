#include "scene_grid.h"
#include <cmath>
namespace megusta {
// 0043ed00: retain the recovered region tests, clamping order and optional
// barycentric outputs. The final absolute value is part of the original.
float pointTriangleDistanceSquared(const SceneTriangleRecord& triangle, const float* point, float* outS, float* outT) {
    const Point3F* v=triangle.vertices;
    float dx=v[0].x-point[0], dy=v[0].y-point[1], dz=v[0].z-point[2];
    float ux=v[1].x-v[0].x, uy=v[1].y-v[0].y, uz=v[1].z-v[0].z;
    float vx=v[2].x-v[0].x, vy=v[2].y-v[0].y, vz=v[2].z-v[0].z;
    float a=ux*ux+uy*uy+uz*uz;
    float b=vx*ux+vy*uy+vz*uz;
    float c=vx*vx+vy*vy+vz*vz;
    float d=dx*ux+dy*uy+dz*uz;
    float e=dx*vx+dy*vy+dz*vz;
    float distance=dx*dx+dy*dy+dz*dz;
    float determinant=std::fabs(static_cast<float>(c*a-b*b));
    float s=e*b-d*c, t=d*b-e*a;
    float parameter, denominator;
    if (determinant < t+s) {
        if (s < 0.0f) {
            parameter=e+c;
            if (parameter <= d+b) {
                if (parameter <= 0.0f) { distance=e+e+distance+c; t=1.0f; s=0.0f; goto done; }
                if (e >= 0.0f) goto origin;
                goto edge_v;
            }
            s=parameter-(d+b); denominator=(a-(b+b))+c;
            if (denominator <= s) { distance=d+d+distance+a; t=0.0f; s=1.0f; goto done; }
            goto opposite_edge_s;
        }
        if (t >= 0.0f) {
            s=((e+c)-b)-d;
            if (s <= 0.0f) { distance=e+e+distance+c; t=1.0f; s=0.0f; goto done; }
            denominator=(a-(b+b))+c;
            if (denominator <= s) { distance=d+d+distance+a; t=0.0f; s=1.0f; goto done; }
            goto opposite_edge_s;
        }
        parameter=d+a;
        if (e+b < parameter) {
            t=parameter-(e+b); denominator=(a-(b+b))+c;
            if (denominator <= t) { t=1.0f; s=0.0f; distance=e+e+distance+c; goto done; }
            t=t/denominator; s=1.0f-t; goto quadratic;
        }
        if (parameter <= 0.0f) { t=0.0f; s=1.0f; distance=d+d+distance+a; goto done; }
        if (d >= 0.0f) goto origin;
    } else {
        if (s >= 0.0f) {
            if (t >= 0.0f) { float inverse=1.0f/determinant; s=inverse*s; t=inverse*t; goto quadratic; }
            if (d >= 0.0f) goto origin;
        } else {
            if (t >= 0.0f) {
                if (e < 0.0f) {
                    if (-e < c) { t=-(e/c); distance=t*e+distance; s=0.0f; }
                    else { distance=e+e+distance+c; t=1.0f; s=0.0f; }
                } else { t=0.0f; s=0.0f; }
                goto done;
            }
            if (d >= 0.0f) {
                if (e >= 0.0f) goto origin;
                if (c <= -e) { distance=e+e+distance+c; t=1.0f; s=0.0f; goto done; }
                goto edge_v;
            }
        }
        t=0.0f;
        if (a <= -d) { s=1.0f; distance=d+d+distance+a; goto done; }
    }
    s=-(d/a); t=0.0f; distance=s*d+distance; goto done;
opposite_edge_s:
    s=s/denominator; t=1.0f-s;
quadratic:
    distance=s*(d+d+s*a+t*b)+t*(e+e+s*b+t*c)+distance;
    goto done;
edge_v:
    t=-(e/c); distance=t*e+distance; s=0.0f; goto done;
origin:
    t=0.0f; s=0.0f;
done:
    if (outS) *outS=s;
    if (outT) *outT=t;
    return std::fabs(distance);
}
namespace {
float pointDistanceAdapter(void*, const SceneTriangleRecord& triangle, const float* point) {
    return pointTriangleDistanceSquared(triangle,point,nullptr,nullptr);
}
}
GridCollisionServices defaultGridCollisionServices() { return {nullptr,pointDistanceAdapter}; }
}
