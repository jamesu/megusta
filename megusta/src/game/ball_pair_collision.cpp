#include "ball.h"
#include "random.h"
#include <cmath>
namespace megusta {
namespace {
long double sqrtFloat(float n) { return std::sqrt(static_cast<long double>(n)); }
float pointLength(const Point3F& p) { return static_cast<float>(sqrtFloat(p.z*p.z+p.y*p.y+p.x*p.x)); }
void normalize(Point3F& p) {
    long double n=sqrtFloat(p.z*p.z+p.y*p.y+p.x*p.x);
    if (n!=0.0L) { n=1.0L/n; p.x=static_cast<float>(n*p.x); p.y=static_cast<float>(n*p.y); p.z=static_cast<float>(n*p.z); }
}
bool intersectForwardRayPlane(const float* plane,const float* start,const Point3F& direction,Point3F& out) {
    float denominator=plane[0]*direction.x+direction.z*plane[2]+plane[1]*direction.y;
    if (denominator==0.0f) return false;
    float numerator=-(plane[0]*start[0]+start[2]*plane[2]+plane[1]*start[1]+plane[3]);
    if ((denominator>0.0f)!=(numerator>0.0f)) return false;
    out.x=(numerator*direction.x)/denominator+start[0];
    out.y=(numerator*direction.y)/denominator+start[1];
    out.z=(numerator*direction.z)/denominator+start[2]; return true;
}
void negate3(Point3F& p) { p.x=-p.x; p.y=-p.y; p.z=-p.z; }
void subtract2(float* a,const float* b) { a[0]-=b[0]; a[1]-=b[1]; }
long double length2(const float* p) { return sqrtFloat(p[1]*p[1]+p[0]*p[0]); }
}
bool Ball::resolveBallCollision(Ball& other,Random& random) {
    if (&other==this || (flags&10u) || (other.flags&10u)) return false;
    Point3F initial{static_cast<float>(position[0]-other.position[0]),static_cast<float>(position[1]-other.position[1]),static_cast<float>(position[2]-other.position[2])};
    float separation=pointLength(initial);
    if (!(separation<=48.0f)) return false;
    Point3F ownMove{static_cast<float>(nextPosition[0]-position[0]),static_cast<float>(nextPosition[1]-position[1]),static_cast<float>(nextPosition[2]-position[2])};
    Point3F otherMove{static_cast<float>(other.nextPosition[0]-other.position[0]),static_cast<float>(other.nextPosition[1]-other.position[1]),static_cast<float>(other.nextPosition[2]-other.position[2])};
    Point3F relative{static_cast<float>(ownMove.x-otherMove.x),static_cast<float>(ownMove.y-otherMove.y),static_cast<float>(ownMove.z-otherMove.z)};
    float squared=relative.x*relative.x+relative.y*relative.y+relative.z*relative.z;
    long double relativeLengthExt=sqrtFloat(squared); float relativeLength=static_cast<float>(relativeLengthExt);
    float reach=relativeLength+1.0;
    if (!(relativeLengthExt>=static_cast<long double>(0.01)) || !(reach>=separation) || !(separation>=1.0f)) return false;
    float approach=(position[0]-other.position[0])*relative.x+relative.y*(position[1]-other.position[1])+relative.z*(position[2]-other.position[2]);
    if (!(approach<=0.0f)) return false;
    Point3F normal=relative; normalize(normal);
    float plane[4]={normal.x,normal.y,normal.z,static_cast<float>(-(normal.x*other.position[0]+normal.y*other.position[1]+normal.z*other.position[2]))};
    Point3F projected;
    if (!intersectForwardRayPlane(plane,position,relative,projected)) return false;
    Point3F offset{static_cast<float>(projected.x-other.position[0]),static_cast<float>(projected.y-other.position[1]),static_cast<float>(projected.z-other.position[2])};
    long double projectionDistance=sqrtFloat(offset.y*offset.y+offset.z*offset.z+offset.x*offset.x);
    if (!(projectionDistance<=1.0L)) return false;
    long double chord=sqrtFloat(static_cast<float>(1.0L-projectionDistance*projectionDistance));
    if (!(chord>=static_cast<long double>(0.001))) return false;
    float projectionFraction=static_cast<float>(-chord/static_cast<long double>(relativeLength));
    projected.x=relative.x*projectionFraction+projected.x;
    projected.y=relative.y*projectionFraction+projected.y;
    projected.z=relative.z*projectionFraction+projected.z;
    Point3F remaining{static_cast<float>((position[0]+relative.x)-projected.x),static_cast<float>((position[1]+relative.y)-projected.y),static_cast<float>((position[2]+relative.z)-projected.z)};
    if (!(remaining.x*relative.x+remaining.y*relative.y+remaining.z*relative.z>=0.0f)) return false;
    float x=position[0]-projected.x, y=position[1]-projected.y, z=position[2]-projected.z;
    long double travelled=sqrtFloat(x*x+z*z+y*y); float fraction=0.0f;
    if (!(static_cast<long double>(relativeLength)<travelled)) fraction=static_cast<float>(travelled/relativeLength);
    if (static_cast<long double>(relativeLength)<travelled || travelled/relativeLength<static_cast<long double>(0.01)) {
        float planar[2]={position[0],position[1]}; subtract2(planar,other.position);
        if (length2(planar)<static_cast<long double>(0.01)) {
            Point3F* movement=position[2]<=other.position[2] ? &otherMove : &ownMove;
            if (sqrtFloat(movement->y*movement->y+movement->x*movement->x)<static_cast<long double>(0.01)) {
                float angle=static_cast<float>(random.generate()%360u)*0.017453292f;
                long double cosine=std::cos(static_cast<long double>(angle));
                float horizontal=static_cast<float>(cosine*movement->z);
                long double sine=std::sin(static_cast<long double>(angle));
                movement->y=static_cast<float>(sine*movement->z); movement->z=0; movement->x=horizontal;
                fraction=1.0f;
            }
        }
    }
    Point3F ownAt,otherAt;
    auto calculate=[&]() {
        ownAt={static_cast<float>(ownMove.x*fraction+position[0]),static_cast<float>(ownMove.y*fraction+position[1]),static_cast<float>(ownMove.z*fraction+position[2])};
        otherAt={static_cast<float>(otherMove.x*fraction+other.position[0]),static_cast<float>(otherMove.y*fraction+other.position[1]),static_cast<float>(otherMove.z*fraction+other.position[2])};
    };
    calculate();
    while (fraction>0.0f) {
        Point3F gap{static_cast<float>(ownAt.x-otherAt.x),static_cast<float>(ownAt.y-otherAt.y),static_cast<float>(ownAt.z-otherAt.z)};
        if (pointLength(gap)>=1.0f) break;
        fraction-=0.1; if (fraction<=0.1) fraction=0.0f; calculate();
    }
    float ownFraction=fraction,otherFraction=fraction;
    if (contact.kind) { velocity[0]+=normalVelocity[0]; velocity[1]+=normalVelocity[1]; velocity[2]+=normalVelocity[2]; ownFraction*=contact.sweepParameter; }
    if (other.contact.kind) { other.velocity[0]+=other.normalVelocity[0]; other.velocity[1]+=other.normalVelocity[1]; other.velocity[2]+=other.normalVelocity[2]; otherFraction*=other.contact.sweepParameter; }
    Point3F contactNormal{static_cast<float>(ownAt.x-otherAt.x),static_cast<float>(ownAt.y-otherAt.y),static_cast<float>(ownAt.z-otherAt.z)};
    float length=pointLength(contactNormal);
    if (length!=0.0f) {
        long double factor=1.0L/static_cast<long double>(length);
        contactNormal.x=static_cast<float>(factor*contactNormal.x);
        contactNormal.y=static_cast<float>(factor*contactNormal.y);
        contactNormal.z=static_cast<float>(factor*contactNormal.z);
    }
    // Earlier decompilation retains the local contact's peer pointers and zero
    // scene indices (recomp/out/ball_object_layout_review.c, 0044c760).
    BallContactResult hit{}; hit.kind=4; hit.sweepParameter=ownFraction;
    hit.position[0]=ownAt.x; hit.position[1]=ownAt.y; hit.position[2]=ownAt.z;
    hit.normal[0]=contactNormal.x; hit.normal[1]=contactNormal.y; hit.normal[2]=contactNormal.z; hit.otherBall=&other;
    resetMotionState(); flags &= ~4u; contact=hit; decomposeContactVelocity(hit.normal);
    nextPosition[0]=ownAt.x; nextPosition[1]=ownAt.y; nextPosition[2]=ownAt.z;
    negate3(contactNormal); hit.sweepParameter=otherFraction;
    hit.position[0]=otherAt.x; hit.position[1]=otherAt.y; hit.position[2]=otherAt.z;
    hit.normal[0]=contactNormal.x; hit.normal[1]=contactNormal.y; hit.normal[2]=contactNormal.z; hit.otherBall=this;
    other.resetMotionState(); other.flags &= ~4u; other.contact=hit; other.decomposeContactVelocity(hit.normal);
    other.nextPosition[0]=otherAt.x; other.nextPosition[1]=otherAt.y; other.nextPosition[2]=otherAt.z;
    return true;
}
}
