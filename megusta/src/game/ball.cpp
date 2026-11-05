#include "ball.h"
#include <cmath>
namespace megusta {
float gameZeroVector[3] = {};
void BallTrailSlot::initialize() { transform=nullptr; selector=nullptr; frame=0; active=false; }
void BallTrailSlot::initializeModels(ssgEntity** models, unsigned count, ssgBranch* root) {
    selector=new ssgSelector(count);
    for (int i=0; i<static_cast<int>(count); ++i) selector->addKid(models[i]);
    transform=new ssgTransform();
    transform->addKid(selector);
    root->addKid(transform);
    reset();
}
void BallTrailSlot::start(sgMat4 matrix) {
    if (!transform) return;
    position[0]=matrix[3][0]; position[1]=matrix[3][1]; position[2]=matrix[3][2];
    transform->setTransform(matrix); transform->setTraversalMaskBits(1);
    frame=0; selector->selectStep(0); active=true;
}
void BallTrailSlot::reset() {
    frame=0; active=false;
    if (transform) { transform->clrTraversalMaskBits(1); selector->selectStep(frame); }
}
void BallTrailSlot::advance() {
    if (transform && active) {
        ++frame;
        if (frame < selector->getMaxKids()) selector->selectStep(frame);
        else reset();
    }
}
void Ball::resetTrail() { for (auto& slot:trail) slot.reset(); trailHead=0; }
void Ball::resetMotionState() {
    flags=(static_cast<std::uint8_t>(flags)&0x72u)|4u;
    rollingContact.kind=0; contact.kind=0; movingStepCount=0; smallMotionStepCount=0;
    resetTrail();
}
void Ball::setLocation(const float* location) {
    position[0]=location[0]; position[1]=location[1]; position[2]=location[2];
    velocity[0]=gameZeroVector[0]; velocity[1]=gameZeroVector[1]; velocity[2]=gameZeroVector[2];
    resetMotionState();
}
void Ball::setVelocity(const float* value) {
    velocity[0]=value[0]; velocity[1]=value[1]; velocity[2]=value[2]; resetMotionState();
}
void Ball::resetOffCourse() { flags=0; setLocation(gameZeroVector); flags=10; restHistoryIndex=0; }
bool Ball::findRestingLocation(const BallRestServices& services) {
    Point3F start{position[0],position[1],static_cast<float>(position[2]+1.0f)};
    Point3F end{position[0],position[1],static_cast<float>(position[2]-12.0f)};
    BallContactResult result;
    if (!services.castSweptBall(services.context,start,end,result)) return false;
    services.backOffBallContact(services.context,start,end,result);
    position[0]=result.position[0]; position[1]=result.position[1]; groundPosition[0]=result.position[0];
    position[2]=result.position[2]; groundPosition[1]=result.position[1]; flags &= 0x12u;
    groundPosition[2]=result.position[2]; rest(services); flags |= 4u; return true;
}
bool Ball::snapToSurface(const float* location, const BallRestServices& services) {
    if (!(flags&1)) return false;
    Point3F start{location[0],location[1],static_cast<float>(location[2]+1.0f)};
    Point3F end{location[0],location[1],static_cast<float>(location[2]-12.0f)};
    BallContactResult result;
    if (!services.castSweptBall(services.context,start,end,result)) return false;
    services.backOffBallContact(services.context,start,end,result);
    position[0]=result.position[0]; position[1]=result.position[1]; position[2]=result.position[2];
    groundPosition[0]=result.position[0]; groundPosition[1]=result.position[1]; groundPosition[2]=result.position[2];
    restHistory[restHistoryIndex].position[0]=result.position[0];
    restHistory[restHistoryIndex].position[1]=result.position[1];
    restHistory[restHistoryIndex].position[2]=result.position[2];
    flags |= 4u; return true;
}
void Ball::rest(const BallRestServices& services) {
    if (flags&1) return;
    flags=(static_cast<std::uint8_t>(flags)&0x12u)|1u;
    velocity[0]=gameZeroVector[0]; velocity[1]=gameZeroVector[1]; velocity[2]=gameZeroVector[2];
    BallRestHistoryEntry& entry=restHistory[restHistoryIndex];
    entry=BallRestHistoryEntry{}; entry.flags=flags;
    entry.position[0]=position[0]; entry.position[1]=position[1]; entry.position[2]=position[2];
    entry.strokeAndPenaltyCount=services.strokeAndPenaltyCount(services.context,playerIndex);
    restHistory[restHistoryIndex].normal[0]=0.0f;
    restHistory[restHistoryIndex].normal[1]=0.0f;
    restHistory[restHistoryIndex].normal[2]=1.0f;
    Point3F start{position[0],position[1],position[2]};
    Point3F end{position[0],position[1],static_cast<float>(position[2]-240.0f)};
    BallContactResult result;
    if (services.castSweptBall(services.context,start,end,result)) {
        groundPosition[0]=result.position[0]; groundPosition[1]=result.position[1]; groundPosition[2]=result.position[2];
        restHistory[restHistoryIndex].supportObjectIndex=result.objectIndex;
        if (0.9 < result.normal[2]) {
            restHistory[restHistoryIndex].normal[0]=result.normal[0];
            restHistory[restHistoryIndex].normal[1]=result.normal[1];
            restHistory[restHistoryIndex].normal[2]=result.normal[2];
        }
    }
}
void Ball::advanceRestHistory(const BallRestServices& services) {
    if (!(flags&1)) services.historyInvariantFailure(services.context);
    ++restHistoryIndex;
    if (restHistoryIndex>3) { restHistory[1]=restHistory[2]; restHistory[2]=restHistory[3]; restHistoryIndex=3; }
    flags &= ~1u; rest(services);
}
bool Ball::restoreRestHistory() {
    if (!(flags&1) || !restHistoryIndex) return false;
    --restHistoryIndex;
    const BallRestHistoryEntry& entry=restHistory[restHistoryIndex];
    lastContactObjectIndex=0; flags=entry.flags;
    position[0]=entry.position[0]; position[1]=entry.position[1]; position[2]=entry.position[2]; flags |= 4u;
    velocity[0]=gameZeroVector[0]; velocity[1]=gameZeroVector[1]; velocity[2]=gameZeroVector[2];
    if (!entry.supportObjectIndex) { if (restHistoryIndex) --restHistoryIndex; resetMotionState(); }
    return true;
}
bool Ball::isRolling() const { return rollingContact.kind!=0; }
void Ball::decomposeContactVelocity(const float* normal) {
    normalVelocity[0]=normal[0]; normalVelocity[1]=normal[1]; normalVelocity[2]=normal[2];
    float dot=velocity[0]*normal[0]+velocity[1]*normal[1]+velocity[2]*normal[2];
    normalVelocity[0]=dot*normalVelocity[0]; normalVelocity[1]=dot*normalVelocity[1]; normalVelocity[2]=dot*normalVelocity[2];
    velocity[0]-=normalVelocity[0]; velocity[1]-=normalVelocity[1]; velocity[2]-=normalVelocity[2];
}
// The original square-root helper takes a float argument and returns x87
// extended precision; explicit casts retain its input rounding boundary.
static long double recoveredSqrt(float value) { return std::sqrt(static_cast<long double>(value)); }
void Ball::rescaleContactSpeed(float sweepParameter, const float* acceleration) {
    long double speed=recoveredSqrt(velocity[2]*velocity[2]+velocity[1]*velocity[1]+velocity[0]*velocity[0]);
    if (speed>0.0L) {
        float x=acceleration[0]*sweepParameter+previousVelocity[0];
        float y=acceleration[1]*sweepParameter+previousVelocity[1];
        float z=acceleration[2]*sweepParameter+previousVelocity[2];
        long double scale=recoveredSqrt(x*x+z*z+y*y)/static_cast<float>(speed);
        velocity[0]=static_cast<float>(scale*velocity[0]); velocity[1]=static_cast<float>(scale*velocity[1]);
        velocity[2]=static_cast<float>(scale*velocity[2]);
    }
}
void applySpeedLoss(float* value, float loss) {
    long double speed=recoveredSqrt(value[2]*value[2]+value[1]*value[1]+value[0]*value[0]);
    if (speed!=0.0L) {
        float x=value[0], y=value[1], z=value[2];
        if (loss<static_cast<float>(speed)) { float scale=loss/static_cast<float>(speed); x*=scale; y*=scale; z*=scale; }
        value[0]-=x; value[1]-=y; value[2]-=z;
    }
}
bool Ball::testMotionThresholds() const {
    float normalSpeed=static_cast<float>(recoveredSqrt(normalVelocity[2]*normalVelocity[2]+normalVelocity[1]*normalVelocity[1]+normalVelocity[0]*normalVelocity[0]));
    if (contact.sweepParameter>=0.01 && normalSpeed>=0.01) return false;
    long double combined=recoveredSqrt(velocity[2]*velocity[2]+velocity[1]*velocity[1]+velocity[0]*velocity[0])+normalSpeed;
    return !(combined < static_cast<long double>(0.1) || combined == static_cast<long double>(0.1));
}
bool Ball::cacheRestingContact(SceneObjectCollection& objects) {
    if ((rollingContact.kind==0 || rollingContact.triangleIndex!=contact.triangleIndex) && contact.kind==3) {
        long double speed=recoveredSqrt(normalVelocity[2]*normalVelocity[2]+normalVelocity[1]*normalVelocity[1]+normalVelocity[0]*normalVelocity[0]);
        if (speed<static_cast<long double>(0.46)) {
            rollingContact=contact; applySpeedLoss(velocity,objects.objects[contact.objectIndex].coefficientOfFriction);
            noteSceneContact(contact,objects); return true;
        }
    }
    return false;
}
void Ball::clearContactTasks() { contactEventCount=0; }
void Ball::queueContactTask(const BallContactEvent& event) {
    if (!event.task || (event.task->stateFlags&1) || contactEventCount>=4) return;
    for (std::uint32_t i=0;i<contactEventCount;++i) if (contactEvents[i].task==event.task) return;
    contactEvents[contactEventCount]=event; ++contactEventCount;
}
void Ball::dispatchContactTasks(TaskRuntime& runtime) {
    float x=position[0], y=position[1], z=position[2];
    flags &= ~4u;
    for (std::uint32_t i=0; i<contactEventCount; ++i) {
        BallContactEvent& event=contactEvents[i];
        position[0]=event.position[0]; position[1]=event.position[1]; position[2]=event.position[2];
        lastContactObjectIndex=event.objectIndex; event.task->start(runtime);
    }
    if (!(flags&4)) { position[0]=x; position[1]=y; position[2]=z; }
    clearContactTasks();
}
void Ball::noteSceneContact(const BallContactResult& result, SceneObjectCollection& objects) {
    if (result.kind==4) return;
    BallContactEvent event; event.objectIndex=result.objectIndex;
    event.task=objects.objects[event.objectIndex].contactTask;
    if (event.task) {
        event.position[0]=result.position[0]; event.position[1]=result.position[1]; event.position[2]=result.position[2];
        queueContactTask(event);
    }
    lastContactObjectIndex=result.objectIndex;
}
SceneObjectRecord* Ball::lastTouch(SceneObjectCollection& objects) const {
    return lastContactObjectIndex ? objects.objects+lastContactObjectIndex : nullptr;
}
void addVectorInPlace(Point3F& value, const Point3F& delta) {
    value.x=delta.x+value.x; value.y=delta.y+value.y; value.z=delta.z+value.z;
}
}
