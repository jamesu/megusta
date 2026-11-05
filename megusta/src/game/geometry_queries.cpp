#include "geometry_queries.h"
#include <cmath>
namespace megusta {
int intersectRaySphere(const Point3F& start, const Point3F& end, const Point3F& center,
                       float radius, Point3F& first, Point3F& second) {
    const float x=end.x-start.x, y=end.y-start.y, z=end.z-start.z;
    const float ox=start.x-center.x, oy=start.y-center.y, oz=start.z-center.z;
    const float a=z*z+y*y+x*x;
    const float b=oz*z+x*ox+y*oy;
    const float discriminant=b*b-((ox*ox+oz*oz+oy*oy)-radius*radius)*a;
    if (0.0f<=discriminant) {
        long double farRoot;
        if (discriminant<=0.0f) farRoot=-static_cast<long double>(b)/a;
        else {
            long double root=std::sqrt(static_cast<double>(discriminant));
            const float nearRoot=static_cast<float>((-static_cast<long double>(b)-root)*(1.0L/a));
            farRoot=(root-b)*(1.0L/a);
            if (nearRoot>=0.0f) {
                first.x=x*nearRoot+start.x; first.y=y*nearRoot+start.y; first.z=z*nearRoot+start.z;
                second.x=static_cast<float>(x*farRoot+start.x);
                second.y=static_cast<float>(y*farRoot+start.y);
                second.z=static_cast<float>(z*farRoot+start.z);
                return 2;
            }
        }
        if (0.0L<=farRoot) {
            first.x=static_cast<float>(x*farRoot+start.x);
            first.y=static_cast<float>(y*farRoot+start.y);
            first.z=static_cast<float>(z*farRoot+start.z);
            return 1;
        }
    }
    return 0;
}
bool clipParametricInequality(float coefficient,float distance,float& begin,float& end) {
    float* update;
    if (coefficient<=0.0f) {
        if (0.0f<=coefficient) return !(0.0f<distance);
        if (coefficient*begin<distance) return false;
        update=&end;
    } else {
        if (coefficient*end<distance) return false;
        update=&begin;
    }
    if (coefficient*(*update)<distance) *update=distance/coefficient;
    return true;
}
bool clipSegmentBoxSlabs(const Point3F& p,const Point3F& d,const Point3F& extents,float& begin,float& end) {
    float oldBegin=begin,oldEnd=end;
    return clipParametricInequality(d.x,-p.x-extents.x,begin,end) &&
           clipParametricInequality(-d.x,p.x-extents.x,begin,end) &&
           clipParametricInequality(d.y,-p.y-extents.y,begin,end) &&
           clipParametricInequality(-d.y,p.y-extents.y,begin,end) &&
           clipParametricInequality(d.z,-p.z-extents.z,begin,end) &&
           clipParametricInequality(-d.z,p.z-extents.z,begin,end) &&
           (begin!=oldBegin || end!=oldEnd);
}
int intersectSegmentOrientedBox(const Point3F& start,const Point3F& end,const Point3F& center,
                                const Point3F& extents,const sgMat4 rotation,Point3F& first,Point3F& second) {
    const float ox=start.x-center.x,oy=start.y-center.y,oz=start.z-center.z;
    const float dx=end.x-start.x,dy=end.y-start.y,dz=end.z-start.z;
    sgVec3 axis={1.0f,0.0f,0.0f};
    sgXformVec3(axis,rotation);
    const float perpendicular=0.0f-axis[1];
    const Point3F local={static_cast<float>(axis[0]*ox+oz*axis[2]+oy*axis[1]),
        static_cast<float>(perpendicular*ox+oy*(axis[0]-0.0f)+oz*0.0f),oz};
    const Point3F direction={static_cast<float>(axis[2]*dz+axis[0]*dx+axis[1]*dy),
        static_cast<float>(dz*0.0f+perpendicular*dx+(axis[0]-0.0f)*dy),dz};
    float begin=0.0f,finish=1.0f;
    if (!clipSegmentBoxSlabs(local,direction,extents,begin,finish)) return 0;
    if (begin<=0.0f) {
        if (1.0f<=finish) return 0;
        first.x=dx*finish+start.x; first.y=dy*finish+start.y; first.z=dz*finish+start.z;
    } else {
        first.x=dx*begin+start.x; first.y=dy*begin+start.y; first.z=dz*begin+start.z;
        if (1.0f>finish) {
            second.x=dx*finish+start.x; second.y=dy*finish+start.y; second.z=dz*finish+start.z;
            return 2;
        }
    }
    return 1;
}
}
