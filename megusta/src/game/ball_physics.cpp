#include "ball_collision.h"
#include <cmath>
#include <cstring>
namespace megusta {
namespace {
Point3F point(const float* p) { return {p[0],p[1],p[2]}; }
long double length3(const float* p) {
    float squared=p[2]*p[2]+p[1]*p[1]+p[0]*p[0];
    return std::sqrt(static_cast<long double>(squared));
}
void assign(float* to,const float* from) { to[0]=from[0]; to[1]=from[1]; to[2]=from[2]; }
void add(float* to,const float* from) { to[0]=from[0]+to[0]; to[1]=from[1]+to[1]; to[2]=from[2]+to[2]; }
}
void Ball::queryMotionContacts(BallPhysicsServices& services) {
    float original[3]={velocity[0],velocity[1],velocity[2]};
    float bestNormal[3]={gameZeroVector[0],gameZeroVector[1],gameZeroVector[2]};
    float friction=0.0f; unsigned count=0;
    if (rollingContact.kind) {
        assign(nextPosition,position); add(nextPosition,services.acceleration);
        if (services.collision.castBallAgainstTriangle(services.objects,rollingContact.triangleIndex,
                                                       point(position),point(nextPosition),contact)) {
            noteSceneContact(rollingContact,services.objects);
            decomposeContactVelocity(rollingContact.normal);
            assign(bestNormal,rollingContact.normal); count=1;
            friction=services.objects.objects[rollingContact.objectIndex].coefficientOfFriction;
        } else rollingContact.kind=0;
    }
    do {
        assign(nextPosition,position); add(nextPosition,velocity);
        if (!services.collision.castSweptBall(services.objects,services.grid,point(position),point(nextPosition),contact)) break;
        decomposeContactVelocity(contact.normal);
        if ((rollingContact.kind && rollingContact.triangleIndex==contact.triangleIndex) || contact.kind!=3 ||
            contact.normal[2]<=0.75 || length3(normalVelocity)>=1.0L) break;
        rollingContact=contact; noteSceneContact(contact,services.objects);
        if (!count || bestNormal[2]<rollingContact.normal[2]) assign(bestNormal,rollingContact.normal);
        ++count; friction+=services.objects.objects[rollingContact.objectIndex].coefficientOfFriction;
    } while (count<5);
    if (!count) return;
    if (count==5) { contact.kind=0; assign(nextPosition,position); return; }
    if (count<2) { applySpeedLoss(normalVelocity,friction); applySpeedLoss(velocity,friction); return; }
    if (contact.kind) add(velocity,normalVelocity);
    float resolved[3]={velocity[0],velocity[1],velocity[2]};
    assign(velocity,original); decomposeContactVelocity(bestNormal);
    applySpeedLoss(velocity,friction/static_cast<float>(count));
    long double target=length3(velocity);
    if (target==0.0L) assign(velocity,gameZeroVector);
    else {
        assign(velocity,resolved); long double current=length3(velocity);
        if (current!=0.0L) {
            long double factor=static_cast<float>(target)/current;
            velocity[0]=static_cast<float>(factor*velocity[0]); velocity[1]=static_cast<float>(factor*velocity[1]);
            velocity[2]=static_cast<float>(factor*velocity[2]);
        }
    }
    if (contact.kind) decomposeContactVelocity(contact.normal);
}
void Ball::beginPhysicsStep(BallPhysicsServices& services) {
    clearContactTasks(); unsigned previousFlags=flags; flags=previousFlags&0xffffff7bu;
    if (!(previousFlags&0x2bu)) {
        ++movingStepCount; assign(previousVelocity,velocity); add(velocity,services.acceleration);
        queryMotionContacts(services);
        if (contact.kind && !resolveSceneImpact(services)) {
            assign(nextPosition,contact.position); assign(groundPosition,contact.position);
        }
    } else { contact.kind=0; assign(nextPosition,position); }
}
bool Ball::resolveSceneImpact(BallPhysicsServices& services) {
    services.collision.backOffBallContact(services.objects,services.grid,point(position),point(nextPosition),contact);
    if (!testMotionThresholds()) return false;
    const int index=contact.objectIndex;
    SceneObjectRecord& object=services.objects.objects[index];
    float restitution=-object.coefficientOfRestitution;
    normalVelocity[0]*=restitution; normalVelocity[1]*=restitution; normalVelocity[2]*=restitution;
    add(velocity,normalVelocity); noteSceneContact(contact,services.objects);
    char sequence[12];
    if (contact.sweepParameter==0.0f && !(object.flags&4u) &&
        services.collision.ballOverlapsGridObject(services.objects,services.grid,contact.objectIndex,point(position)))
        std::strcpy(sequence,services.overlapRecovery);
    else sequence[0]='\0';
    float square=previousVelocity[1]*previousVelocity[1]+previousVelocity[0]*previousVelocity[0];
    long double horizontal=std::sqrt(static_cast<long double>(square));
    std::strcat(sequence,horizontal<=0.5L ? services.slowRecovery : services.fastRecovery);
    const char* step=sequence;
    while (*step) {
        Point3F start=point(position);
        if (*step=='0') object.flags |= 1u;
        else if (*step=='1') { start.z+=0.1; nextPosition[2]+=0.1; }
        else if (*step=='2') {
            start.z+=0.1; nextPosition[0]=start.x; nextPosition[1]=start.y; nextPosition[2]=start.z;
            add(nextPosition,previousVelocity);
        } else {
            assign(normalVelocity,contact.normal);
            normalVelocity[0]*=0.075; normalVelocity[1]*=0.075; normalVelocity[2]*=0.075;
            assign(nextPosition,position); add(nextPosition,normalVelocity);
        }
        if (!services.collision.castSweptBall(services.objects,services.grid,start,point(nextPosition),contact)) break;
        decomposeContactVelocity(contact.normal);
        if (!cacheRestingContact(services.objects)) {
            services.collision.backOffBallContact(services.objects,services.grid,start,point(nextPosition),contact);
            if (!testMotionThresholds()) { assign(nextPosition,contact.position); break; }
            noteSceneContact(contact,services.objects); add(velocity,normalVelocity);
        }
        ++step;
    }
    if (!*step) { assign(nextPosition,position); contact.kind=0; }
    object.flags &= ~1u; flags |= 0x80u; rollingContact.kind=0; contact.sweepParameter=0.0f;
    return true;
}
void Ball::resolveContactResponse(BallPhysicsServices& services) {
    if (flags&0x2fu) return;
    if (contact.kind) {
        BallContactResult hit=contact;
        if (hit.kind==4) {
            if (hit.otherBall->contact.kind==4) {
                if (static_cast<unsigned>(playerIndex)<static_cast<unsigned>(hit.otherBall->playerIndex)) {
                    services.playBallImpact(services.context,hit.position);
                    float saved[3]={normalVelocity[0],normalVelocity[1],normalVelocity[2]};
                    assign(normalVelocity,hit.otherBall->normalVelocity); assign(hit.otherBall->normalVelocity,saved);
                }
                normalVelocity[0]*=0.8; normalVelocity[1]*=0.8; normalVelocity[2]*=0.8;
            } else { normalVelocity[0]*=-0.8; normalVelocity[1]*=-0.8; normalVelocity[2]*=-0.8; }
            add(velocity,normalVelocity);
        } else {
            SceneObjectRecord& object=services.objects.objects[hit.objectIndex];
            services.playObjectImpact(services.context,object,hit.position,normalVelocity);
            if (!isRolling() || (flags&0x80u)) {
                add(velocity,normalVelocity); rescaleContactSpeed(hit.sweepParameter,services.acceleration);
                decomposeContactVelocity(hit.normal);
            }
            float bounce=-object.coefficientOfRestitution;
            normalVelocity[0]*=bounce; normalVelocity[1]*=bounce; normalVelocity[2]*=bounce;
            add(velocity,normalVelocity); noteSceneContact(hit,services.objects);
        }
    }
    float x=position[0], y=position[1], z=position[2];
    assign(position,nextPosition); flags |= 4u;
    float dx=x-nextPosition[0], dy=y-nextPosition[1], dz=z-nextPosition[2];
    float squared=dx*dx+dz*dz+dy*dy;
    long double distance=std::sqrt(static_cast<long double>(squared));
    if (distance>=static_cast<long double>(0.1)) {
        smallMotionStepCount=0;
        const float* low=services.grid.bounds.getMin(); const float* high=services.grid.bounds.getMax();
        if (position[0]<low[0] || high[0]<position[0] || position[1]<low[1] ||
            high[1]<position[1] || position[2]<low[2]) services.dispatchTerminalEvent(services.context);
    } else {
        ++smallMotionStepCount;
        if (smallMotionStepCount<10) { if (smallMotionStepCount>4) rollingContact.kind=0; }
        else rest(services.rest);
    }
    if (!(flags&1u) && movingStepCount>0x708u) rest(services.rest);
}
}
