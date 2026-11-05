#include "camera.h"
#include "game_coordinate_system.h"
#include "player.h"
#include "balls.h"
#include "putter.h"
#include "spatial_object.h"
#include "scene_grid.h"
#include "ground_shadow.h"
#include "native_game_helpers.h"
#include "engine/engine_math.h"
#include "engine/opengl_adapter.h"
#include "engine/resource_runtime.h"
#include "engine/string_services.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
namespace {
bool zero(const Point3F& point) { return std::fabs(point.x)<1e-6L && std::fabs(point.y)<1e-6L && std::fabs(point.z)<1e-6L; }
Point3F delta(const Point3F& a,const Point3F& b) { return {static_cast<float>(a.x-b.x),static_cast<float>(a.y-b.y),static_cast<float>(a.z-b.z)}; }
float square(const Point3F& v) { return v.x*v.x+v.y*v.y+v.z*v.z; }
Point3F point(const float* value) { return {value[0],value[1],value[2]}; }
Point3F blend(const Point3F& first,const Point3F& second,float amount) {
    return {static_cast<float>((second.x-first.x)*amount+first.x),static_cast<float>((second.y-first.y)*amount+first.y),static_cast<float>((second.z-first.z)*amount+first.z)};
}
Point3F orbit(const Point3F& target,float pitch,float heading,float distance) {
    const float pitchAngle=pitch*0.017453292f,headingAngle=heading*-0.017453292f;
    const float horizontal=static_cast<float>(engine_math::engineMath_cos_impl(pitchAngle)*distance);
    const float x=static_cast<float>(engine_math::engineMath_cos_impl(headingAngle)*horizontal),y=static_cast<float>(engine_math::engineMath_sin_impl(headingAngle)*horizontal);
    const float z=static_cast<float>(engine_math::engineMath_sin_impl(pitchAngle)*distance);
    return {static_cast<float>(target.x+x),static_cast<float>(target.y+y),static_cast<float>(target.z+z)};
}
long double headingAngle(const Point3F& direction) {
    return Camera::headingFromOrbitVector(direction);
}
bool following(CameraFrameServices& s) { return Camera::canFollowMovingBall(s.runtime,s.objects.sceneRoot!=nullptr,s.players.currentPlayer,s.balls,s.putter); }
CameraConstraintServices constraintServices(Camera& camera,CameraFrameServices& s) {
    return {&s,camera.pitchDegrees,camera.followDistance,[](void*){return isEditing();},
        [](void* context,SpatialRepel& repel){auto& frame=*static_cast<CameraFrameServices*>(context); repel.rebuildHeightGrid(frame.objects,frame.grid);}};
}
}
bool Camera::turnTowardBall(CameraRuntime& runtime,const Point3F& origin) {
    const auto direction=delta(runtime.ballPosition,origin); if (zero(direction)) return false;
    smoothHeading(static_cast<float>(headingAngle(direction)-180)); return true;
}
bool Camera::headingFromPlayerTransform(const Player& player) {
    sgVec3 direction={0,1,0}; sgXformVec3(direction,player.characterTransform);
    long double angle=std::asin(static_cast<long double>(direction[1]))*57.29578L;
    if (direction[0]>0) angle=180-angle; headingDegrees=static_cast<float>(angle);
    if (player.leftHanded) headingDegrees+=180; return true;
}
bool Camera::resetOrbitForPlayer(const Player& player) { headingFromPlayerTransform(player); pitchDegrees=19.8f; followDistance=40; return true; }
bool Camera::turnAlongBallVelocity(CameraFrameServices& s) {
    auto* ball=s.players.currentPlayer->getBall(s.balls); if (!following(s)) return false;
    const auto direction=point(ball->velocity); if (zero(direction)) return false;
    smoothHeading(static_cast<float>(headingAngle(direction))); return true;
}
void Camera::applyInputMotion(CameraRuntime& runtime) {
    runtime.headingVelocity+=runtime.headingAcceleration; runtime.pitchVelocity+=runtime.pitchAcceleration;
    const auto clamp=[](float value) { if (value<2) { if (value<=-2) value=-2; } else value=2; return value; };
    runtime.headingVelocity=clamp(runtime.headingVelocity); runtime.pitchVelocity=clamp(runtime.pitchVelocity);
    headingDegrees+=runtime.headingVelocity; pitchDegrees+=runtime.pitchVelocity; clampEditorOrbit(isEditing);
}
void Camera::handleInput(CameraFrameServices& s) {
    auto& r=s.runtime; auto& input=s.input; const unsigned buttons=input.buttons;
    bool zoom=false;
    if (s.hasTabElements) { s.platform.setMouseCapture(s.captureOption?16:15); mouseCaptured=0; return; }
    if (state==CameraMode::Control || state==CameraMode::Frozen) { mouseCaptured=0; r.headingAcceleration=r.headingVelocity=r.pitchVelocity=0; return; }
    if (!isEditing()) {
        if (!(buttons&0xc0a00c)) r.headingVelocity=r.headingAcceleration=0;
        else {
            r.headingAcceleration=(buttons&0x8000)?-0.02f:0.02f;
            if (buttons&0xc) r.headingAcceleration=(buttons&4)?-2:2;
            if (buttons&0xc00000) r.headingVelocity=input.joystickAxes[0]+input.joystickAxes[0]; mouseCaptured=0;
        }
        if (!(buttons&0x305000)) r.pitchVelocity=0; else { r.pitchVelocity=(buttons&0x101000)?2:-2; mouseCaptured=0; }
        if ((buttons&0x10) && !(input.previousButtons&0x10)) zoom=true;
    }
    if (mouseCaptured) {
        if ((buttons&0x2000000) && !(input.previousButtons&0x2000000)) r.zoomDragMoved=false;
        if (!(buttons&0x2000000) && (input.previousButtons&0x2000000) && !r.zoomDragMoved) { zoom=true; mouseCaptured=0; }
        else if (!(buttons&0x2000000) || (buttons&0x1000000)) {
            if (s.putter.state!=2 && s.putter.state!=3) { pitchDegrees+=(input.mouse.y-r.previousMouseY)*0.3f; headingDegrees+=(input.mouse.x-r.previousMouseX)*0.3f; clampEditorOrbit(isEditing); }
        } else {
            followDistance+=(input.mouse.y-r.previousMouseY)*0.1f; if (input.mouse.y!=r.previousMouseY) r.zoomDragMoved=true;
            if (s.putter.state==1) headingDegrees+=(input.mouse.x-r.previousMouseX)*0.3f; clampEditorOrbit(isEditing);
        }
    }
    clampEditorOrbit(isEditing);
    const auto capture=[&] { r.previousMouseX=s.screenWidth/2; r.previousMouseY=s.screenHeight/2;
        s.warpCursor(r.previousMouseX,r.previousMouseY); s.platform.setMouseCapture(16); mouseCaptured=1; };
    if (!mouseCaptured) capture();
    else if (input.mouse.x<100 || input.mouse.x>s.screenWidth-100 || input.mouse.y<100 || input.mouse.y>s.screenHeight-100) { mouseCaptured=0; capture(); }
    else { r.previousMouseX=input.mouse.x; r.previousMouseY=input.mouse.y; }
    if (zoom) {
        s.playSound("camzoom",0);
        if (state==CameraMode::Internal13 || !zero(r.transitionVector)) { r.transitionVector={0,0,0}; setState(r,CameraMode::NoAutomatic); followDistance=70; }
        else if (followDistance<70) followDistance=70;
        else if (followDistance<100) followDistance=100; else if (followDistance<130) followDistance=130; else state=CameraMode::Internal13;
    }
    clampEditorOrbit(isEditing);
}
void Camera::buildViewMatrices(CameraFrameServices& s) {
    // 00437360 gates the framing offset with DAT_0052ce6c: targetBallWithOffset
    // sets it, while targetCharacterHeight/targetBallPosition clear it. Use the
    // recovered per-frame state instead of the old app-level boolean, which
    // was permanently false and left the camera aimed too low at the ball.
    auto target=getFramingTarget(isEditing(),s.runtime.ballTargetOffset!=0); auto eye=controlEye; sgVec3 up={0,0,1};
    if (s.fixedTick-s.runtime.jitterStartTick<static_cast<unsigned>(s.runtime.jitterDuration)) {
        const auto jitter=[](float scale){return static_cast<float>(static_cast<int>(std::rand()&15)-8)*scale;};
        up[0]+=jitter(0.0078125f); up[1]+=jitter(0.0078125f); up[2]+=jitter(0.0078125f);
        eye.x+=jitter(0.03125f); eye.y+=jitter(0.03125f); eye.z+=jitter(0.03125f);
        target.x+=jitter(0.0625f); target.y+=jitter(0.0625f); target.z+=jitter(0.0625f);
    }
    sgVec3 e={eye.x,eye.y,eye.z},t={target.x,target.y,target.z}; sgMat4 matrix,conversion,projection;
    sgMakeLookAtMat4(matrix,e,t,up); auto* context=ssgGetCurrentContext(); context->setCamera(matrix);
    sgTransposeNegateMat4(viewMatrix,matrix); std::memcpy(conversion,gameCoordinateConversionMatrix,sizeof(conversion)); sgPostMultMat4(viewMatrix,conversion);
    viewBasis[0]=matrix[0][0]; viewBasis[2]=matrix[0][1]; viewBasis[6]=matrix[0][2];
    viewBasis[1]=matrix[1][0]; viewBasis[3]=matrix[1][1]; viewBasis[7]=matrix[1][2];
    viewBasis[4]=matrix[2][0]; viewBasis[5]=matrix[2][1]; viewBasis[8]=matrix[2][2];
    context->getProjectionMatrix(projection); screenProjection[0]=(s.screenWidth/2)*projection[0][0]; screenProjection[1]=(s.screenHeight/2)*projection[1][1];
    screenProjection[2]=static_cast<float>(s.screenWidth/2); screenProjection[3]=static_cast<float>(s.screenHeight/2);
}
void Camera::selectSpatialRegionCamera(CameraFrameServices& s) {
    auto& r=s.runtime; if (!following(s) || state==CameraMode::Sky || s.typeFilter==1) return;
    SpatialRegion* selected=nullptr; int priority=-999;
    const auto candidate=[&](SpatialObject* object,SpatialRegion* owner,float extra) {
        const auto difference=delta(point(object->position()),r.ballPosition); const float radius=object->radius()+extra;
        if (square(difference)<radius*radius && (priority==-999 || priority<difference.z)) { priority=static_cast<int>(difference.z); selected=owner; }
    };
    for (auto* object=s.spatial.head;object;object=s.spatial.next(object)) {
        s.spatial.iterator=object; if (std::strcmp(object->typeTag(),"SpatialRegion")) continue;
        auto* region=static_cast<SpatialRegion*>(object); bool linkedCamera=false;
        for (int id:region->linkedIds) if (auto* linked=s.spatial.findByIdPointer(&id)) if (!strings::compareInsensitive(linked->typeTag(),"SpatialCamera")) { linkedCamera=true; break; }
        if (!linkedCamera) continue;
        if (r.activeRegion==region && r.activeSpatialCamera) {
            candidate(region,region,20);
            for (int id:r.activeRegion->linkedIds) if (auto* linked=s.spatial.findByIdPointer(&id)) if (!strings::compareInsensitive(linked->typeTag(),"SpatialZone")) candidate(linked,region,0);
        } else candidate(region,region,0);
    }
    s.spatial.iterator=nullptr;
    if (!selected) {
        r.activeRegion=nullptr; r.activeSpatialCamera=nullptr;
        if (state!=CameraMode::NoAutomatic && state!=CameraMode::Sky) { captureOrbitHeading(); setState(r,CameraMode::NoAutomatic); }
        return;
    }
    if (r.activeRegion && r.activeRegion!=selected) captureOrbitHeading();
    SpatialCamera* nearest=nullptr; float distance=-999;
    for (int id:selected->linkedIds) if (auto* linked=s.spatial.findByIdPointer(&id)) {
        if (strings::compareInsensitive(linked->typeTag(),"SpatialCamera")) continue;
        const float value=square(delta(point(linked->position()),r.ballPosition));
        if (distance==-999 || value<distance) { distance=value; nearest=static_cast<SpatialCamera*>(linked); }
    }
    if (r.activeSpatialCamera==nearest && r.activeRegion==selected) return;
    r.activeRegion=selected; r.activeSpatialCamera=nearest; nearest->flags_|=1;
    r.spatialEye=point(nearest->position()); r.regionCenter=point(selected->position()); r.previousSpatialEye=controlEye;
    r.regionBallPosition=r.ballPosition; r.spatialBlend=0;
    for (int id:selected->linkedIds) if (auto* linked=s.spatial.findByIdPointer(&id)) if (!strings::compareInsensitive(linked->typeTag(),"SpatialZone")) r.regionCenter=point(linked->position());
    switch (nearest->flags_&0xff00) {
    case 0: setState(r,CameraMode::Secure); break;
    case 0x100: setState(r,CameraMode::Frozen); controlFramePosition=r.spatialEye; controlDirection=r.regionCenter; break;
    case 0x200: setState(r,CameraMode::Ball); break;
    case 0x400: case 0x500: setState(r,CameraMode::Internal13); break;
    case 0x600: setState(r,CameraMode::Internal15); break;
    case 0x700: setState(r,CameraMode::Internal16); break;
    default: setState(r,CameraMode::Sky); break;
    }
}
void Camera::resolveOrbitAndRepulsion(CameraFrameServices& s,const Point3F& target,unsigned flags) {
    auto& r=s.runtime;
    s.players.currentPlayer->getBall(s.balls); following(s);
    if (isEditing() || (flags&8)) {
        if (isEditing()) controlDirection=r.ballPosition;
        resolvedPitchDegrees=pitchDegrees; resolvedHeadingDegrees=headingDegrees; resolvedFollowDistance=followDistance;
        controlFramePosition=orbit(target,resolvedPitchDegrees,resolvedHeadingDegrees,resolvedFollowDistance); controlEye=controlFramePosition; controlLookAt=controlDirection; return;
    }
    float heading=headingDegrees,pitch=pitchDegrees,distance=followDistance;
    if (!(flags&0x20)) {
        controlFramePosition=orbit(target,pitch,heading,distance);
        if (s.typeFilter!=1) {
            auto* saved=s.spatial.iterator; auto services=constraintServices(*this,s);
            for (int pass=0;pass<2;++pass) for (auto* object=s.spatial.head;object;object=s.spatial.next(object)) {
                s.spatial.iterator=object;
                if (strings::compareInsensitive(object->typeTag(),"SpatialRepel") || (((object->flags_&0xff00)==0x300)!=(pass==1))) continue;
                static_cast<SpatialRepel*>(object)->applyCameraConstraint(controlFramePosition,controlDirection,target,pitch,distance,heading,services);
                controlFramePosition=orbit(target,pitch,heading,distance);
            }
            s.spatial.iterator=saved;
        }
    }
    resolvedFollowDistance=distance; resolvedHeadingDegrees=heading; resolvedPitchDegrees=pitch;
    if (!(flags&0x20)) controlFramePosition=orbit(target,pitch,heading,distance);
    if (flags&4) {
        if (following(s) && s.putter.state!=1) flags=1;
        if (!zero(r.transitionVector)) flags|=1;
    }
    ++r.orbitBlendFrame;
    if (!(flags&1) || r.orbitBlendFrame>19) controlEye=controlFramePosition;
    else controlEye=blend(controlEye,controlFramePosition,r.orbitBlendFrame*0.05f);
    if ((flags&1) && r.orbitBlendFrame<20) controlLookAt=blend(controlLookAt,controlDirection,r.orbitBlendFrame*0.05f); else controlLookAt=controlDirection;
}
void Camera::updateState(CameraFrameServices& s) {
    auto& r=s.runtime; auto* player=s.players.currentPlayer; if (!player) return;
    bool select=false;
    switch (state) {
    case CameraMode::NoAutomatic: case CameraMode::Automatic: case CameraMode::Ball: case CameraMode::Secure:
    case CameraMode::Frozen: case CameraMode::Cinema: case CameraMode::Internal16: select=true; break;
    case CameraMode::Sky: {
        if (square(delta(controlLookAt,point(player->characterTransform[3])))>900) { captureOrbitHeading(); setState(r,CameraMode::NoAutomatic); }
        select=state!=CameraMode::Sky; break;
    }
    case CameraMode::Control: if (!r.controlPathObject) returnToAutomatic(r); else advanceControlPath(r,s.animation,s.platform); select=state!=CameraMode::Control; break;
    case CameraMode::Internal17: select=s.fixedTick-r.overrideStartTick>=r.overrideDuration; break;
    default: break;
    }
    if (select) selectSpatialRegionCamera(s);
    Point3F target; unsigned flags=0; bool resolve=true;
    switch (state) {
    case CameraMode::NoAutomatic: case CameraMode::Automatic: targetBallWithOffset(r); target=controlDirection; flags=4; break;
    case CameraMode::Sky: targetBallWithOffset(r); [[fallthrough]];
    case CameraMode::Frozen: case CameraMode::Internal17: target=r.regionBallPosition; flags=35; break;
    case CameraMode::Ball: targetBallPosition(r); turnTowardBall(r,r.regionCenter); followDistance=static_cast<float>(std::sqrt(static_cast<long double>(square(r.spatialEye)))); target=controlDirection; break;
    case CameraMode::Secure: targetBallPosition(r); controlFramePosition=r.spatialEye; target=controlDirection; flags=52; break;
    case CameraMode::Player: targetCharacterHeight(r,*player); resetOrbitForPlayer(*player); target=controlDirection; flags=3; break;
    case CameraMode::Cinema: targetBallPosition(r); if (!turnAlongBallVelocity(s)) restartFollow(r); target=controlDirection; break;
    case CameraMode::Out: case CameraMode::Internal13: case CameraMode::Internal14: case CameraMode::Internal16: {
        targetBallWithOffset(r); const auto tee=point(s.putter.teeObject->getPosition()); Point3F center=tee; float count=1; char name[32];
        for (int i=0;i<16;++i) { std::sprintf(name,"tr_cup_%02d",i); auto* object=s.objects.findByName(name);
            if (object && object->name && !(object->flags&1)) { const auto p=point(object->getPosition()); center.x+=p.x; center.y+=p.y; center.z+=p.z; count+=1; } }
        center.x/=count; center.y/=count; center.z/=count; controlLookAt=controlDirection=controlEye=controlFramePosition=center;
        controlEye.x+=60; controlEye.y+=60; const auto difference=delta(tee,center);
        controlEye.z+=zero(difference)?250:static_cast<float>(std::sqrt(static_cast<long double>(square(difference)))*3);
        controlFramePosition=controlEye;
        if (state!=CameraMode::Internal16) for (auto* object=s.spatial.head;object;object=s.spatial.next(object)) {
            s.spatial.iterator=object;
            if (!std::strcmp(object->typeTag(),"SpatialCamera") && (object->flags_&0x400)) {
                auto* camera=static_cast<SpatialCamera*>(object); controlFramePosition=controlEye=point(camera->position()); controlDirection=controlLookAt=point(camera->lookAt_);
            }
        }
        s.spatial.iterator=nullptr; target=controlDirection; flags=48; break;
    }
    case CameraMode::Internal15: {
        Point3F positions[8]; int count=0;
        if (r.activeRegion && r.activeSpatialCamera) for (int id:r.activeRegion->linkedIds)
            if (auto* linked=s.spatial.findByIdPointer(&id)) if (!strings::compareInsensitive(linked->typeTag(),"SpatialCamera")) positions[count++]=point(linked->position());
        // The saved routine repeats all adjacent blends count times. When the
        // linked list is empty it reads the unwritten local positions[0]; keep
        // that original edge behavior instead of substituting a new exception.
        for (int pass=0;pass<count;++pass) for (int i=0;i<count-1;++i) {
            positions[i].x=(1-r.spatialBlend)*positions[i].x+r.spatialBlend*positions[i+1].x;
            positions[i].y=(1-r.spatialBlend)*positions[i].y+r.spatialBlend*positions[i+1].y;
            positions[i].z=(1-r.spatialBlend)*positions[i].z+r.spatialBlend*positions[i+1].z;
        }
        controlFramePosition=r.spatialEye=positions[0]; targetBallPosition(r); resolveOrbitAndRepulsion(s,controlDirection,52); resolve=false;
        r.spatialBlend+=0.02f; if (r.spatialBlend>=1) { setState(r,CameraMode::Secure); return; } break;
    }
    default: resolve=false; break;
    }
    if (resolve) resolveOrbitAndRepulsion(s,target,flags); buildViewMatrices(s);
}
void Camera::updateFollowTarget(CameraFrameServices& s) {
    auto& r=s.runtime; auto* player=s.players.currentPlayer;
    if (player) {
        if (!r.headingRestoreRequested) player->controlOption94=static_cast<unsigned>(headingDegrees);
        if (auto* ball=player->getBall(s.balls)) {
            if (s.putter.state==1 || following(s)) r.ballPosition=point(ball->position);
            else {
                s.players.findNext(nullptr); float maximum=-1;
                for (auto* other=s.players.findNextOrFirst(nullptr);other;other=s.players.findNextOrFirst(other)) if (other!=player && other->getBall(s.balls)) {
                    auto* moving=other->getBall(s.balls); const float speed=moving->velocity[2]*moving->velocity[2]+moving->velocity[1]*moving->velocity[1]+moving->velocity[0]*moving->velocity[0];
                    if (speed!=0 && (maximum<speed || maximum==-1)) { r.ballPosition=point(moving->position); maximum=speed; }
                }
            }
            setNodeAimVector(r.ballPosition.x,r.ballPosition.y,r.ballPosition.z);
            if (!zero(r.transitionVector)) {
                r.ballPosition=r.transitionVector;
                if (r.followEntity) { (void)r.followEntity->getSpare(); r.followEntity->setTraversalMaskBits(1); }
            } else if (r.followEntity) r.followEntity->clrTraversalMaskBits(1);
        }
    }
    updateState(s);
}
void Camera::resetFollowState(CameraFrameServices& s) {
    auto& r=s.runtime; clearGroundShadowCache(); // The intervening netc_serviceEnd_stub() in 00439fa0 is a verified empty return.
    followDistance=resolvedFollowDistance=70; pitchDegrees=resolvedPitchDegrees=19.8f;
    r.transitionVector={0,0,0}; r.overrideStartTick=r.jitterStartTick=s.fixedTick; r.overrideDuration=0; r.jitterDuration=0; setState(r,CameraMode::NoAutomatic);
    if (!restoreSavedHeadingAtRest(*s.players.currentPlayer->getBall(s.balls)) && r.headingAvailable) headingDegrees=static_cast<float>(s.players.currentPlayer->controlOption94);
    r.headingRestoreRequested=false; r.headingAvailable=true; resolvedHeadingDegrees=headingDegrees; resolvedPitchDegrees=pitchDegrees; resolvedFollowDistance=followDistance;
    controlEye=controlFramePosition; controlLookAt=controlDirection;
    for (s.spatial.iterator=s.spatial.head;s.spatial.iterator;s.spatial.iterator=s.spatial.next(s.spatial.iterator)) {
        if (std::strcmp(s.spatial.iterator->typeTag(),"SpatialRegion")) continue;
        for (int id:static_cast<SpatialRegion*>(s.spatial.iterator)->linkedIds) if (auto* linked=s.spatial.findByIdPointer(&id))
            if (!strings::compareInsensitive(linked->typeTag(),"SpatialCamera")) linked->flags_&=0xfffe;
    }
    r.activeSpatialCamera=nullptr;
}
void Camera::drawEditorTargetMarker(CameraFrameServices& s) const {
    if (!isEditing()) return; const auto& target=s.runtime.transitionVector;
    glMatrixMode(GL_MODELVIEW); glLoadMatrixf(&viewMatrix[0][0]); glTranslatef(target.x,target.y,target.z);
    glDisable(GL_CULL_FACE); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D); glColor4f(0,0,0,1);
    s.gl.drawSphere(1,10,5); s.gl.drawCylinder(1,3,10,10,5); glEnable(GL_CULL_FACE); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); glEnable(GL_TEXTURE_2D);
}
void Camera::dump(CameraFrameServices& s) const {
    sharedIo->stdioPrintf("camera.autocam = %d\n",static_cast<int>(state));
    sharedIo->stdioPrintf("camera.fov = %.2f\n",static_cast<double>(fieldOfView));
    sharedIo->stdioPrintf("camera.loc = (%.2f,%.2f,%.2f)\n",static_cast<double>(controlEye.x),static_cast<double>(controlEye.y),static_cast<double>(controlEye.z));
    sharedIo->stdioPrintf("ball.loc = (%.2f,%.2f,%.2f)\n",static_cast<double>(s.runtime.ballPosition.x),static_cast<double>(s.runtime.ballPosition.y),static_cast<double>(s.runtime.ballPosition.z));
    auto* ball=s.players.currentPlayer->getBall(s.balls);
    const auto* lastTouch=ball->lastTouch(s.objects);
    sharedIo->stdioPrintf("ball.getLastTouch() = %s\n",lastTouch?lastTouch->name:"NULL");
}
}
