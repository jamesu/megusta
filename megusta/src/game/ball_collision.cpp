#include "ball_collision.h"
#include <cmath>
namespace megusta {
BallCollisionWorkspace ballCollision{};
static long double sqrtFloat(float x) { return std::sqrt(static_cast<long double>(x)); }
static float pointLength(const Point3F& p) { return static_cast<float>(sqrtFloat(p.z*p.z+p.y*p.y+p.x*p.x)); }
static void scale(Point3F& p, float n) { p.x*=n; p.y*=n; p.z*=n; }
static void setLength(Point3F& p, float n) {
    long double length=sqrtFloat(p.z*p.z+p.y*p.y+p.x*p.x);
    if (length!=0.0L) {
        long double factor=static_cast<long double>(n)/length;
        p.x=static_cast<float>(factor*p.x); p.y=static_cast<float>(factor*p.y); p.z=static_cast<float>(factor*p.z);
    }
}
static long double triple(const Point3F& a,const Point3F& b,const Point3F& c) {
    return (static_cast<long double>(c.y)*b.x-static_cast<long double>(c.x)*b.y)*a.z +
        ((static_cast<long double>(c.z)*b.y-static_cast<long double>(b.z)*c.y)*a.x -
         (static_cast<long double>(c.z)*b.x-static_cast<long double>(b.z)*c.x)*a.y);
}
void BallCollisionWorkspace::setSweep(const Point3F& from,const Point3F& to) {
    start=from; end=to;
    delta.x=to.x-from.x; delta.y=to.y-from.y; delta.z=to.z-from.z;
    long double magnitude=sqrtFloat(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    length=static_cast<float>(magnitude);
    if (magnitude>0.0L) {
        direction=delta;
        magnitude=sqrtFloat(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
        if (magnitude!=0.0L) {
            long double factor=1.0L/magnitude;
            direction.x=static_cast<float>(direction.x*factor);
            direction.y=static_cast<float>(direction.y*factor);
            direction.z=static_cast<float>(direction.z*factor);
        }
        backoffFraction=static_cast<float>(0.1/length);
    } else { direction={0,0,0}; backoffFraction=1.0f; }
}
long double BallCollisionWorkspace::distanceToEdgeAtFraction(const Point3F& a,const Point3F& b,float fraction) {
    result.sweepParameter=fraction;
    result.position[0]=delta.x*fraction+start.x; result.position[1]=delta.y*fraction+start.y;
    result.position[2]=delta.z*fraction+start.z;
    Point3F relative{static_cast<float>(result.position[0]-a.x),static_cast<float>(result.position[1]-a.y),static_cast<float>(result.position[2]-a.z)};
    Point3F edge{static_cast<float>(b.x-a.x),static_cast<float>(b.y-a.y),static_cast<float>(b.z-a.z)};
    long double edgeLength=sqrtFloat(edge.x*edge.x+edge.y*edge.y+edge.z*edge.z);
    long double normalLength=sqrtFloat(edge.x*edge.x+edge.y*edge.y+edge.z*edge.z);
    if (normalLength!=0.0L) scale(edge,static_cast<float>(1.0L/normalLength));
    float along=edge.x*relative.x+edge.y*relative.y+edge.z*relative.z;
    if (along>=0.0f) {
        Point3F nearest;
        if (along<=static_cast<float>(edgeLength)) {
            nearest.x=edge.x*along+a.x; nearest.y=edge.y*along+a.y; nearest.z=edge.z*along+a.z;
        } else nearest=b;
        result.normal[0]=result.position[0]-nearest.x; result.normal[1]=result.position[1]-nearest.y;
        result.normal[2]=result.position[2]-nearest.z;
    } else { result.normal[0]=relative.x; result.normal[1]=relative.y; result.normal[2]=relative.z; }
    return sqrtFloat(result.normal[0]*result.normal[0]+result.normal[1]*result.normal[1]+result.normal[2]*result.normal[2]);
}
bool BallCollisionWorkspace::testTriangleInteriorOrEdge(const Point3F& point,const SceneTriangleServices& services) {
    Point3F vertices[3]; triangle.copyVertices(vertices,services);
    unsigned i=0; Point3F* previous=&vertices[2]; Point3F* current=&vertices[0]; Point3F edge;
    for (;;) {
        edge={static_cast<float>(current->x-previous->x),static_cast<float>(current->y-previous->y),static_cast<float>(current->z-previous->z)};
        float plane[3]; triangle.copyPlane(plane,services);
        Point3F side{static_cast<float>(plane[2]*edge.y-plane[1]*edge.z),
                     static_cast<float>(edge.z*plane[0]-plane[2]*edge.x),
                     static_cast<float>(plane[1]*edge.x-edge.y*plane[0])};
        float sideLength=pointLength(side);
        if (sideLength!=0.0f) scale(side,static_cast<float>(1.0/sideLength));
        float distance=(point.x-previous->x)*side.x+(point.y-previous->y)*side.y+(point.z-previous->z)*side.z;
        // The original leaves the loop for positive or unordered results.
        if (!(distance<0.0f) && !(distance==0.0f)) break;
        if (++i>2) return true;
        previous=current; ++current;
    }
    float fraction;
    if (length<=0.0f) fraction=0.0f;
    else {
        float edgeLength=pointLength(edge);
        if (edgeLength!=0.0f) scale(edge,static_cast<float>(1.0/edgeLength));
        Point3F cross{static_cast<float>(edge.y*direction.z-edge.z*direction.y),
                      static_cast<float>(edge.z*direction.x-edge.x*direction.z),
                      static_cast<float>(edge.x*direction.y-edge.y*direction.x)};
        float square=cross.x*cross.x+cross.y*cross.y+cross.z*cross.z;
        if (square==0.0f) return false;
        Point3F relative{static_cast<float>(start.x-previous->x),static_cast<float>(start.y-previous->y),static_cast<float>(start.z-previous->z)};
        long double parameter=triple(relative,direction,cross)/square;
        Point3F onEdge{static_cast<float>(static_cast<float>(edge.x*parameter)+previous->x),
                       static_cast<float>(static_cast<float>(edge.y*parameter)+previous->y),
                       static_cast<float>(parameter*edge.z+previous->z)};
        float along=static_cast<float>(triple(relative,edge,cross)/square);
        Point3F onSweep{static_cast<float>(direction.x*along+start.x),static_cast<float>(direction.y*along+start.y),static_cast<float>(direction.z*along+start.z)};
        long double separation=sqrtFloat((onEdge.x-onSweep.x)*(onEdge.x-onSweep.x)+(onEdge.z-onSweep.z)*(onEdge.z-onSweep.z)+(onEdge.y-onSweep.y)*(onEdge.y-onSweep.y));
        if (static_cast<long double>(0.5)+static_cast<long double>(0.1)<separation) return false;
        if (along>=0.0f) fraction=along<=length ? along/length : 1.0f; else fraction=0.0f;
    }
    long double distance=distanceToEdgeAtFraction(*previous,*current,fraction);
    if (distance<static_cast<long double>(0.5)) result.kind=2;
    else if (!result.kind) return false;
    Point3F normal{result.normal[0],result.normal[1],result.normal[2]}; setLength(normal,1.0f);
    result.normal[0]=normal.x; result.normal[1]=normal.y; result.normal[2]=normal.z;
    return false;
}
bool BallCollisionWorkspace::testSweptSphereTriangle(SceneObjectCollection& objects) {
    result.kind=0;
    if (objects.objects[triangle.objectIndex].flags&1u) return false;
    SceneTriangleServices services=objects.triangleServices();
    float plane[3]; long double distance=triangle.copyPlane(plane,services);
    if (static_cast<long double>(start.x)*plane[0]+static_cast<long double>(plane[1])*start.y+
        static_cast<long double>(plane[2])*start.z < -distance) {
        plane[0]=-plane[0]; plane[1]=-plane[1]; plane[2]=-plane[2]; distance=-distance;
    }
    float planeDistance=static_cast<float>(distance);
    Point3F radiusNormal{plane[0],plane[1],plane[2]};
    float magnitude=pointLength(radiusNormal);
    if (magnitude!=0.0f) scale(radiusNormal,static_cast<float>(0.5/magnitude));
    Point3F nearStart{static_cast<float>(start.x-radiusNormal.x),static_cast<float>(start.y-radiusNormal.y),static_cast<float>(start.z-radiusNormal.z)};
    float startDot=nearStart.x*plane[0]+nearStart.y*plane[1]+nearStart.z*plane[2];
    if (startDot < -planeDistance) {
        planeDistance=start.x*plane[0]+plane[1]*start.y+plane[2]*start.z+planeDistance;
        float factor=-planeDistance;
        Point3F projection{static_cast<float>(plane[0]*factor+start.x),static_cast<float>(plane[1]*factor+start.y),static_cast<float>(plane[2]*factor+start.z)};
        if (testTriangleInteriorOrEdge(projection,services)) {
            result.normal[0]=plane[0]; result.normal[1]=plane[1]; result.normal[2]=plane[2]; result.kind=1;
            result.position[0]=start.x; result.position[1]=start.y; result.position[2]=start.z; result.sweepParameter=planeDistance;
        }
        return result.kind!=0;
    }
    float endDot=(end.x-radiusNormal.x)*plane[0]+(end.y-radiusNormal.y)*plane[1]+(end.z-radiusNormal.z)*plane[2];
    if (endDot<startDot) {
        float motionDot=delta.x*plane[0]+plane[1]*delta.y+plane[2]*delta.z;
        if (motionDot!=0.0f) {
            float fraction=-((startDot+planeDistance)/motionDot);
            if (fraction>=0.0f && fraction<=1.0f) {
                Point3F point{static_cast<float>(delta.x*fraction),static_cast<float>(delta.y*fraction),static_cast<float>(delta.z*fraction)};
                addVectorInPlace(point,nearStart);
                if (testTriangleInteriorOrEdge(point,services)) {
                    addVectorInPlace(point,radiusNormal);
                    result.sweepParameter=fraction; result.normal[0]=plane[0]; result.normal[1]=plane[1]; result.normal[2]=plane[2];
                    result.position[0]=point.x; result.kind=3; result.position[1]=point.y; result.position[2]=point.z;
                    return true;
                }
                return result.kind!=0;
            }
        }
    }
    return false;
}
bool BallCollisionWorkspace::castSweptBall(SceneObjectCollection& objects,SceneGridIndex& grid,
                                            const Point3F& from,const Point3F& to,BallContactResult& out) {
    out.kind=0; setSweep(from,to); grid.collectSegmentCandidates(objects,from,to);
    for (int i=0;i<grid.candidateCount;++i) {
        unsigned id=grid.candidateIds[i]; triangle=objects.triangleStorage[id];
        if (testSweptSphereTriangle(objects)) {
            result.triangleIndex=static_cast<int>(id); result.objectIndex=triangle.objectIndex; result.otherBall=nullptr;
            if (!out.kind || result.sweepParameter<out.sweepParameter) out=result;
        }
    }
    return out.kind!=0;
}
bool BallCollisionWorkspace::castBallAgainstTriangle(SceneObjectCollection& objects,int index,
                    const Point3F& from,const Point3F& to,BallContactResult& out) {
    out.kind=0; setSweep(from,to); triangle=objects.triangleStorage[index];
    if (testSweptSphereTriangle(objects)) {
        result.objectIndex=triangle.objectIndex; result.triangleIndex=index; result.otherBall=nullptr; out=result;
    }
    return out.kind!=0;
}
bool BallCollisionWorkspace::ballOverlapsGridObject(SceneObjectCollection& objects,SceneGridIndex& grid,
                                                    int objectIndex,const Point3F& position) {
    if (!objectIndex) return false;
    setSweep(position,position); grid.collectObjectTriangles(objects,objectIndex);
    for (int i=0;i<grid.candidateCount;++i) {
        triangle=objects.triangleStorage[grid.candidateIds[i]];
        if (testSweptSphereTriangle(objects)) return true;
    }
    return false;
}
void BallCollisionWorkspace::backOffBallContact(SceneObjectCollection& objects,SceneGridIndex& grid,
                                                const Point3F& from,const Point3F& to,BallContactResult& inout) {
    setSweep(from,to);
    if (!inout.kind) { inout.sweepParameter=1.0f; inout.position[0]=end.x; inout.position[1]=end.y; inout.position[2]=end.z; }
    else if (inout.sweepParameter==0.0f) return;
    const Point3F savedStart=start, savedDelta=delta;
    const float step=backoffFraction;
    Point3F current{inout.position[0],inout.position[1],inout.position[2]};
    setSweep(current,current);
    const auto backOff=[&]() -> bool {
        while (testSweptSphereTriangle(objects)) {
            float fraction=inout.sweepParameter-step; inout.sweepParameter=fraction;
            if (fraction<=step) {
                inout.sweepParameter=0.0f; inout.position[0]=from.x; inout.position[1]=from.y; inout.position[2]=from.z;
                return false;
            }
            inout.position[0]=savedDelta.x; inout.position[1]=savedDelta.y; inout.position[2]=savedDelta.z;
            inout.position[0]=fraction*inout.position[0]; inout.position[1]=fraction*inout.position[1]; inout.position[2]=fraction*inout.position[2];
            inout.position[0]=savedStart.x+inout.position[0]; inout.position[1]=savedStart.y+inout.position[1]; inout.position[2]=savedStart.z+inout.position[2];
            current={inout.position[0],inout.position[1],inout.position[2]}; setSweep(current,current);
        }
        return true;
    };
    if (inout.kind) { triangle=objects.triangleStorage[inout.triangleIndex]; if (!backOff()) return; }
    grid.collectSegmentCandidates(objects,from,to);
    for (int i=0;i<grid.candidateCount;++i) {
        triangle=objects.triangleStorage[grid.candidateIds[i]];
        if (!backOff()) return;
    }
}
namespace {
bool restCast(void* opaque,const Point3F& from,const Point3F& to,BallContactResult& result) {
    auto& context=*static_cast<BallRestContext*>(opaque);
    return context.collision.castSweptBall(context.objects,context.grid,from,to,result);
}
void restBackOff(void* opaque,const Point3F& from,const Point3F& to,BallContactResult& result) {
    auto& context=*static_cast<BallRestContext*>(opaque);
    context.collision.backOffBallContact(context.objects,context.grid,from,to,result);
}
int restScore(void* opaque,int playerIndex) {
    auto& context=*static_cast<BallRestContext*>(opaque);
    return context.strokeAndPenaltyCount(context.playerContext,playerIndex);
}
void restFatal(void* opaque) {
    auto& context=*static_cast<BallRestContext*>(opaque);
    context.historyInvariantFailure(context.playerContext);
}
}
BallRestServices makeBallRestServices(BallRestContext& context) {
    return {&context,restCast,restBackOff,restScore,restFatal};
}
}
