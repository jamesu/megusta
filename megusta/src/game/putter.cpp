#include "putter.h"
#include "player.h"
#include "balls.h"
#include "random.h"
#include "putt_map_runtime.h"
#include "putt_guide.h"
#include "scene_grid.h"
#include "game_shape_adapter.h"
#include "room_client.h"
#include "camera.h"
#include "sounds.h"
#include "effect.h"
#include "../engine/multi_texture.h"
#include <cmath>
#include <cstdio>
namespace megusta {
namespace {
// The original occlusion ramp is DAT_0052bfdc, accessed by a free function,
// so keep it outside the Putter instance layout.
float putterOcclusionLevel=0.0f;
}
void Putter::dump() const {
    std::printf("putter.state = %d\n",state);
    std::printf("putter.power = %.2f\n",static_cast<double>(power));
}
long double evaluatePutterPowerCurve(float value,const PutterTuning& tuning) {
    return static_cast<long double>(tuning.powerCurveQuadratic)*value*value+tuning.powerCurveOffset;
}
void Putter::handleInputMode2(PutterHumanInputServices& s) {
    Player& player=*s.players.currentPlayer;
    Ball* ball=player.getBall(s.balls);
    if (!(ball->flags&1) || (ball->flags&10) || (ball->flags&0x20)) {
        if (state!=0 && state!=7 && state!=8 && state!=5 && state!=6 && state!=2) setAnimation(0,s.animation);
        animationIdleInterval=1800; animationIdleStart=s.tick; gestureStart=s.tick; gestureInterval=0; return;
    }
    for (std::uint32_t i=0;i<s.players.playerCount;++i) {
        Player& other=s.players.playerStorage[i];
        if (!other.isDone(s.balls) && !(other.getBall(s.balls)->flags&1)) return;
    }
    // Preserve the equality test shown in the recovered mode-2 body.
    if (s.buttons || aimHeading==previousAimAngle) {
        previousAimAngle=aimHeading; animationIdleStart=s.tick; animationIdleInterval=1800; gestureStart=s.tick; gestureInterval=0;
        if ((player.shapeAdapter.playbackFlags&1) && animationId==3) setAnimation(0,s.animation);
    }
    if (state==1) {
        bool waiting=s.client&&s.client->shouldWaitForRemoteTurn();
        bool ready=!s.client||s.client->isTurnStateReady();
        aimHeading=s.cameraHeading; captureInputAnchor(waiting,player.isHuman,s.mouseX,s.mouseY);
        if ((s.buttons&0x02000000u) || waiting || !ready) return;
        bool mousePress=((s.buttons&~s.previousButtons)&0x01000000u)!=0;
        if (s.joystickAxis3>0 || (mousePress && s.mouseY>5)) {
            previousJoystickAxis=0;
            if (s.camera.stopControlCam(s.cameraRuntime)) { state=9; return; }
            if (mousePress) { previousDragMouseY=0; dragStartY=static_cast<float>(s.mouseY)-3.0f; }
            else dragStartY=0;
            setAnimation(0,s.animation); state=2; return;
        }
        if (!(player.shapeAdapter.playbackFlags&1)) {
            if (s.tick-animationIdleStart>=animationIdleInterval) {
                setAnimation(3,s.animation); animationIdleStart=s.tick;
                animationIdleInterval=((s.animationRandom.generate()%20u)*3u+120u)*10u; return;
            }
            if (s.tick-gestureStart>=gestureInterval) {
                gestureStart=s.tick; gestureInterval=(s.animationRandom.generate()%20u)*30u;
                setAnimation((s.animationRandom.generate()&1u) ? 4 : 5,s.animation);
            }
        }
    } else if (state==2) {
        aimHeading=s.cameraHeading;
        Point3F origin{ball->position[0],ball->position[1],static_cast<float>(ball->position[2]+0.2)};
        Point3F travel{smoothedAim[0],smoothedAim[1],smoothedAim[2]};
        float squared=travel.x*travel.x+travel.y*travel.y+travel.z*travel.z;
        long double length=std::sqrt(static_cast<long double>(squared));
        if (length!=0) {
            long double factor=-static_cast<long double>(s.guideLengthScale)/length;
            travel.x=static_cast<float>(factor*travel.x); travel.y=static_cast<float>(factor*travel.y); travel.z=static_cast<float>(factor*travel.z);
        }
        s.guide.updateTrajectory(origin,travel,s.objects,s.grid,s.collision);
    } else if (state==4) {
        aimHeading=s.cameraHeading;
        if (s.tick-idleStart>=idleInterval) { idleStart=s.tick; idleInterval=15; state=9; }
    } else if (state==9) {
        setAnimation((s.animationRandom.generate()&1u) ? 4 : 5,s.animation); state=1; inputGuardFrames=10;
    }
}
void Putter::updateMode2(PutterControlServices& s) {
    auto& input=s.humanInput;
    switch (state) {
    case 1: aimHeading=s.cameraHeading; return;
    case 2: {
        aimHeading=s.cameraHeading;
        float value=0;
        if (dragStartY<=0) {
            if (input.joystickAxis3<previousJoystickAxis-0.15) { state=5; s.player.shapeAdapter.startPlayback(s.shape); return; }
            if (input.joystickAxis3<=0) { state=9; return; }
            s.player.shapeAdapter.startPlayback(s.shape);
            value=static_cast<float>((input.joystickAxis3-0.1)*1.1111112);
            previousJoystickAxis=input.joystickAxis3;
        } else {
            float drag=static_cast<float>((static_cast<float>(input.mouseY)-dragStartY-dragDeadZone)/dragRange);
            if (!((input.buttons&0x02000000u) || !(input.previousButtons&0x02000000u)) || drag>1.1) {
                input.buttons=input.previousButtons; effectivePower=power; state=9; return;
            }
            if (static_cast<float>(input.mouseY)<static_cast<float>(previousDragMouseY)-10.0f) { state=5; s.player.shapeAdapter.startPlayback(s.shape); return; }
            s.player.shapeAdapter.startPlayback(s.shape); value=drag; previousDragMouseY=input.mouseY;
        }
        if (value>=1) value=1; else if (value<=0) value=0;
        power=value; float time=static_cast<float>((1.0-value)*conditionalPowerStep);
        if (time>=conditionalPowerStep) time=conditionalPowerStep; else if (time<=0) time=0;
        s.player.shapeAdapter.setTime(time,s.shape); return;
    }
    case 5: {
        effectivePower=power;
        long double curve=evaluatePutterPowerCurve(power,s.tuning);
        strikeBall(static_cast<float>(-curve*shotSpeedScale),s.strike); state=6; return;
    }
    case 6:
        if (conditionalPowerStep<=s.player.shapeAdapter.getTime(s.shape)) { s.player.shapeAdapter.playbackFlags&=~1u; state=0; idleStart=s.fixedTick; idleInterval=0; }
        return;
    case 7: if (s.player.getBall(s.balls)->flags&1) { setAnimation(7,s.animation); state=8; } return;
    case 8: if (!(s.player.shapeAdapter.playbackFlags&1)) state=0; return;
    case 9: power=0; effectivePower=0; previousJoystickAxis=0; previousDragMouseY=0; return;
    default: return;
    }
}
void Putter::updateMode0(PutterControlServices& s) {
    switch (state) {
    case 2: {
        aimHeading=s.cameraHeading;
        float scale=static_cast<float>(0.03-s.player.controlAttribute*0.015);
        float increment=static_cast<float>(1.0/(conditionalPowerStep*60.0)),scaled=scale*effectivePower;
        if (scaled<increment) increment=scaled<=scale ? scale : scaled;
        effectivePower+=increment;
        if (effectivePower>=1.0f) { effectivePower=0; state=9; }
        float time=static_cast<float>((1.0-effectivePower)*conditionalPowerStep);
        if (time>=conditionalPowerStep) time=conditionalPowerStep; else if (time<=0) time=0;
        s.player.shapeAdapter.setTime(time,s.shape); return;
    }
    case 3: {
        float scale=static_cast<float>(0.03-s.player.accuracy*0.015);
        float factor=static_cast<float>(power*power*1.6+0.3);
        float limit=static_cast<float>(1.0/(conditionalPowerStep*60.0)),minimum=scale*factor,scaled=factor*scale*effectivePower;
        if (scaled<limit) limit=scaled<=minimum ? minimum : scaled;
        effectivePower-=limit;
        if (effectivePower<=0) { effectivePower=0; state=5; }
        float time=static_cast<float>((1.0-effectivePower)*conditionalPowerStep);
        if (time>=conditionalPowerStep) time=conditionalPowerStep; else if (time<=0) time=0;
        s.player.shapeAdapter.setTime(time,s.shape); return;
    }
    case 5: {
        if (s.player.shapeAdapter.getTime(s.shape)<static_cast<long double>(conditionalPowerStep)-0.1L) return;
        float angle=static_cast<float>((1.5-s.player.accuracy)*10.0);
        float scale=static_cast<float>(0.03-s.player.accuracy*0.015);
        float step=static_cast<float>(angle/(0.109999985/scale));
        if (power>=0.28f) {
            bool good=false;
            if (effectivePower<=0.15f) {
                if (effectivePower>0.12f) {
                    s.sounds.play("goodhit",0); if(effect) static_cast<Effect*>(effect)->warmup(0);
                    if(effect) static_cast<Effect*>(effect)->start(); effectFrames=20; good=true;
                } else if (effectivePower>0) {
                    float high=step+angle;
                    shotAngleOffset=static_cast<float>(high-(effectivePower/scale)*step);
                }
            } else if (effectivePower<0.27f) {
                shotAngleOffset=static_cast<float>(((0.26-effectivePower)/scale)*step-(step+angle));
            } else shotAngleOffset=-angle;
            if (!good) s.sounds.play("swing2",0);
        }
        long double curve=evaluatePutterPowerCurve(power,s.tuning);
        strikeBall(static_cast<float>(-curve*shotSpeedScale),s.strike); state=6; return;
    }
    case 6:
        // The recovered mode-1 body exits follow-through only on the sequence
        // time reaching conditionalPowerStep; unlike modes 0 and 2 it does not
        // also test the playback flag.
        if (conditionalPowerStep<=s.player.shapeAdapter.getTime(s.shape)) {
            s.player.shapeAdapter.playbackFlags&=~1u; state=0; idleStart=s.fixedTick; idleInterval=0;
        }
        return;
    case 7:
        if (s.player.getBall(s.balls)->flags&1) { setAnimation(7,s.animation); state=8; }
        return;
    case 8: if (!(s.player.shapeAdapter.playbackFlags&1)) state=0; return;
    case 9: power=0; effectivePower=0; aimHeading=s.cameraHeading; return;
    default: return;
    }
}
void Putter::handleInputMode0(PutterHumanInputServices& s) {
    Player& player=*s.players.currentPlayer;
    Ball* ball=player.getBall(s.balls);
    if (!(ball->flags&1) || (ball->flags&10) || (ball->flags&0x20)) {
        if (state!=0 && state!=7 && state!=8 && state!=5 && state!=6) setAnimation(0,s.animation);
        animationIdleInterval=1800; animationIdleStart=s.tick; gestureStart=s.tick; gestureInterval=0; return;
    }
    for (std::uint32_t i=0;i<s.players.playerCount;++i) {
        Player& other=s.players.playerStorage[i];
        if (!other.isDone(s.balls) && !(other.getBall(s.balls)->flags&1)) return;
    }
    if (s.buttons || aimHeading!=previousAimAngle) {
        previousAimAngle=aimHeading; animationIdleStart=s.tick; animationIdleInterval=1800; gestureStart=s.tick; gestureInterval=0;
        if ((player.shapeAdapter.playbackFlags&1) && animationId==3) setAnimation(0,s.animation);
    }
    bool press=((s.buttons & ~s.previousButtons)&(0x40u|0x01000000u))!=0;
    if (state==1) {
        bool waiting=s.client&&s.client->shouldWaitForRemoteTurn();
        bool ready=!s.client||s.client->isTurnStateReady();
        aimHeading=s.cameraHeading; captureInputAnchor(waiting,player.isHuman,s.mouseX,s.mouseY);
        if ((s.buttons&0x02000000u) || waiting || !ready) return;
        if (press) {
            if (!s.camera.stopControlCam(s.cameraRuntime)) { setAnimation(0,s.animation); state=2; s.sounds.play("swing1",0); }
            else state=9;
            return;
        }
        if (!(player.shapeAdapter.playbackFlags&1)) {
            if (s.tick-animationIdleStart>=animationIdleInterval) {
                setAnimation(3,s.animation); animationIdleStart=s.tick;
                animationIdleInterval=((s.animationRandom.generate()%20u)*3u+120u)*10u; return;
            }
            if (s.tick-gestureStart>=gestureInterval) {
                gestureStart=s.tick; gestureInterval=(s.animationRandom.generate()%20u)*30u;
                setAnimation((s.animationRandom.generate()&1u) ? 4 : 5,s.animation);
            }
        }
    } else if (state==2) {
        aimHeading=s.cameraHeading;
        if (!(s.buttons&0x02000000u) && press) {
            power=effectivePower; state=3; s.sounds.play("swing2",0);
            if (power<0.28f) { state=5; player.shapeAdapter.startPlayback(s.shape); }
        }
    } else if (state==3 && press) { state=5; player.shapeAdapter.startPlayback(s.shape); }
    else if (state==9) {
        aimHeading=s.cameraHeading; setAnimation((s.animationRandom.generate()&1u) ? 4 : 5,s.animation);
        state=1; inputGuardFrames=10;
    }
}
void Putter::handleInputMode1(PutterHumanInputServices& s) {
    Player& player=*s.players.currentPlayer;
    Ball* ball=player.getBall(s.balls);
    if (!(ball->flags&1) || (ball->flags&10) || (ball->flags&0x20)) {
        animationIdleStart=s.tick; animationIdleInterval=1800; gestureStart=s.tick; gestureInterval=0; return;
    }
    for (std::uint32_t i=0;i<s.players.playerCount;++i) {
        Player& other=s.players.playerStorage[i];
        if (!other.isDone(s.balls) && !(other.getBall(s.balls)->flags&1)) return;
    }
    if (s.client&&s.client->shouldWaitForRemoteTurn()) {
        if (state==2 && (power>=s.remotePowerThreshold || s.tick-animationIdleStart>=animationIdleInterval)) {
            state=5; player.shapeAdapter.startPlayback(s.shape);
        }
        return;
    }
    if (s.client&&!s.client->isTurnStateReady()) return;
    if (s.buttons || aimHeading!=previousAimAngle) {
        previousAimAngle=aimHeading; animationIdleStart=s.tick; animationIdleInterval=1800; gestureStart=s.tick; gestureInterval=0;
        if ((player.shapeAdapter.playbackFlags&1) && animationId==3) setAnimation(0,s.animation);
    }
    bool pressed=(s.buttons&(0x40u|0x01000000u|0x20000u))!=0;
    if (state==1) {
        aimHeading=s.cameraHeading; captureInputAnchor(false,player.isHuman,s.mouseX,s.mouseY);
        if (inputGuardFrames>0) { --inputGuardFrames; return; }
        if (s.buttons&0x02000000u) return;
        if (pressed) {
            if (!s.camera.stopControlCam(s.cameraRuntime)) { setAnimation(0,s.animation); state=2; s.sounds.play("swing1",0); }
            else state=9;
            return;
        }
        if (!(player.shapeAdapter.playbackFlags&1)) {
            if (s.tick-animationIdleStart>=animationIdleInterval) {
                setAnimation(3,s.animation); animationIdleStart=s.tick;
                animationIdleInterval=((s.animationRandom.generate()%20u)*3u+120u)*10u; return;
            }
            if (s.tick-gestureStart>=gestureInterval) {
                gestureStart=s.tick; gestureInterval=(s.animationRandom.generate()%20u)*30u;
                setAnimation((s.animationRandom.generate()&1u) ? 4 : 5,s.animation); return;
            }
        }
    } else if (state==2 || state==4) {
        aimHeading=s.cameraHeading;
        if (!pressed) { state=5; player.shapeAdapter.startPlayback(s.shape); s.sounds.play("swing2",0); }
    } else if (state==9 && !pressed) { state=1; inputGuardFrames=10; }
}
void Putter::updateMode1(PutterControlServices& s) {
    switch (state) {
    case 2: {
        aimHeading=s.cameraHeading;
        float scale=s.player.isHuman ? 0.0225f : 0.1f;
        float increment=static_cast<float>(1.0/(conditionalPowerStep*60.0)),scaled=scale*power;
        if (scaled<increment) increment=scaled<=scale ? scale : scaled;
        power+=increment;
        if (power>=1.0f) { power=1; state=4; idleStart=s.fixedTick; idleInterval=15; }
        effectivePower=power;
        float time=static_cast<float>((1.0-power)*conditionalPowerStep);
        if (time>=conditionalPowerStep) time=conditionalPowerStep; else if (time<=0) time=0;
        s.player.shapeAdapter.setTime(time,s.shape); return;
    }
    case 4:
        aimHeading=s.cameraHeading;
        if (s.fixedTick-idleStart>=idleInterval) { idleStart=s.fixedTick; idleInterval=15; state=9; }
        return;
    case 5:
        if (conditionalPowerStep<=s.player.shapeAdapter.getTime(s.shape)) {
            long double curve=evaluatePutterPowerCurve(power,s.tuning);
            strikeBall(static_cast<float>(-curve*shotSpeedScale),s.strike); state=6;
        }
        return;
    case 6:
        if (conditionalPowerStep<=s.player.shapeAdapter.getTime(s.shape)) { s.player.shapeAdapter.playbackFlags&=~1u; state=0; idleStart=s.fixedTick; idleInterval=0; }
        return;
    case 7:
        if (s.player.getBall(s.balls)->flags&1) { setAnimation(7,s.animation); state=8; }
        return;
    case 8: if (!(s.player.shapeAdapter.playbackFlags&1)) state=0; return;
    case 9:
        aimHeading=s.cameraHeading;
        if (s.fixedTick-idleStart>=idleInterval) {
            s.player.shapeAdapter.playbackFlags&=~1u; long double time=s.player.shapeAdapter.getTime(s.shape);
            s.player.shapeAdapter.setTime(static_cast<float>((static_cast<long double>(conditionalPowerStep)-time)*0.25L+time),s.shape);
            power=0; effectivePower=0;
        }
        return;
    default: return;
    }
}
void Putter::drawAimPreview(PutterAimServices& s) {
    if (!s.player) return;
    Ball* ball=s.player->getBall(s.balls);
    if (state!=1 && state!=2) { s.guide.hideGeometry(); return; }
    aimHeading=s.cameraHeading; networkServiceEnd();
    Point3F aim;
    const bool waiting=s.client&&s.client->shouldWaitForRemoteTurn();
    if (!waiting) { computeShotVector(1.0f,aim,*s.player,s.balls); if(s.client)s.client->sendAimVector(aim); }
    else aim={aimTarget[0],aimTarget[1],aimTarget[2]};
    float components[3]={aim.x,aim.y,aim.z};
    if (resetAimSmoothing) { resetAimSmoothing=false; for (int i=0;i<3;++i) smoothedAim[i]=components[i]; }
    else if (waiting) {
        for (int i=0;i<3;++i) {
            float delta=components[i]-smoothedAim[i];
            if (delta>=0.1f) delta=0.1f; else if (delta<=-0.1f) delta=-0.1f;
            smoothedAim[i]+=delta;
        }
    } else for (int i=0;i<3;++i) smoothedAim[i]=components[i];
    float squared=smoothedAim[0]*smoothedAim[0]+smoothedAim[1]*smoothedAim[1]+smoothedAim[2]*smoothedAim[2];
    float length=static_cast<float>(std::sqrt(static_cast<long double>(squared)));
    Point3F forward{smoothedAim[0],smoothedAim[1],smoothedAim[2]};
    if (length!=0) { float factor=static_cast<float>(1.0/length); forward.x*=factor; forward.y*=factor; forward.z*=factor; }
    const float* n=ball->restHistory[ball->restHistoryIndex].normal;
    Point3F side{static_cast<float>(n[2]*forward.y-forward.z*n[1]),static_cast<float>(forward.z*n[0]-n[2]*forward.x),static_cast<float>(n[1]*forward.x-n[0]*forward.y)};
    long double sideLength=std::sqrt(static_cast<long double>(side.z*side.z+side.y*side.y+side.x*side.x));
    if (sideLength>0.1L) {
        if (sideLength!=0) { float factor=static_cast<float>(1.0L/sideLength); side.x*=factor; side.y*=factor; side.z*=factor; }
        float (*matrix)[4]=s.player->characterTransform;
        matrix[0][0]=side.x; matrix[0][1]=side.y; matrix[0][2]=side.z; matrix[0][3]=0;
        matrix[1][0]=forward.x; matrix[1][1]=forward.y; matrix[1][2]=forward.z; matrix[1][3]=0;
        matrix[2][0]=n[0]; matrix[2][1]=n[1]; matrix[2][2]=n[2]; matrix[2][3]=0;
        matrix[3][0]=matrix[3][1]=matrix[3][2]=0; matrix[3][3]=1;
        sgMat4 rotation; sgMakeCoordMat4(rotation,0,0,0,270,0,0); sgPostMultMat4(matrix,rotation);
        matrix[3][0]=ball->position[0]; matrix[3][1]=ball->position[1]; matrix[3][2]=ball->position[2]; matrix[3][3]=1;
    }
    if (state==1 && (!s.client || s.client->canAdvanceSimulation())) {
        Point3F origin{ball->position[0],ball->position[1],static_cast<float>(ball->position[2]+0.2)};
        Point3F travel=forward;
        float v=forward.x*forward.x+forward.z*forward.z+forward.y*forward.y;
        long double magnitude=std::sqrt(static_cast<long double>(v));
        if (magnitude!=0) {
            long double factor=static_cast<long double>(-(s.player->accuracy*static_cast<float>(s.guideLengthScale)))/magnitude;
            travel.x=static_cast<float>(factor*travel.x); travel.y=static_cast<float>(factor*travel.y); travel.z=static_cast<float>(factor*travel.z);
        }
        s.guide.updateTrajectory(origin,travel,s.objects,s.grid,s.collision); return;
    }
    s.guide.hideGeometry();
}
void Putter::drawCharacterPass1(PutterDrawServices& s) {
    s.player.shapeAdapter.drawPreview(s.cameraView,s.player.characterTransform[0],s.player.leftHanded);
}
void Putter::drawCharacterPass2(PutterDrawServices& s) {
    s.player.shapeAdapter.drawShadow(s.cameraView,s.player.characterTransform[0],s.player.leftHanded);
    networkServiceEnd();
}
void Putter::drawCharacterOcclusionPass(PutterDrawServices& s) {
    auto collision=defaultGridCollisionServices();
    for (int i=0;i<2;++i) {
        Point3F point{0,static_cast<float>(s.player.leftHanded ? -10.0f : 10.0f),0};
        sgVec3 transformed; sgVec3 local={point.x,point.y,point.z};
        sgXformPnt3(transformed,local,s.player.characterTransform);
        point.x=transformed[0]; point.y=transformed[1]; point.z=transformed[2];
        point.z+=i==0 ? 10.1f : 20.1f;
        if (s.grid.resolveSphereTrianglePenetration(s.objects,point,10.0f,false,collision) && putterOcclusionLevel<100.0f) putterOcclusionLevel+=2.0f;
    }
    if (putterOcclusionLevel!=0.0f) {
        putterOcclusionLevel-=1.0f;
        s.player.shapeAdapter.drawWithAlpha(s.cameraView,s.player.characterTransform[0],
            s.player.leftHanded,putterOcclusionLevel*0.005f);
    }
}
void showPutterPowerCurve(const PutterTuning& tuning,PlayersManager& players,std::uint32_t tick) {
    char message[128];
    std::snprintf(message,sizeof(message),"F = P * P * %.2f + %.2f",static_cast<double>(tuning.powerCurveQuadratic),static_cast<double>(tuning.powerCurveOffset));
    players.showMessage(message,90,tick);
}
void Putter::releaseHoleResources() {
    pix::destroyMultiTexture(static_cast<pix::MultiTextureList*>(hudAtlas)); hudAtlas=nullptr;
    if (effect) {
        auto* emitter=static_cast<Effect*>(effect);
        emitter->releaseTexture(); delete emitter; effect=nullptr;
    }
}
void Putter::loadHoleResources(PutterResourceServices& s) {
    s.guide.attachSceneRoots(s.sceneRoot,s.secondarySceneRoot); state=0; idleStart=s.tick; idleInterval=0;
    animationIdleStart=s.tick; animationIdleInterval=1800; gestureStart=s.tick; gestureInterval=0;
    teeObject=nullptr; powerBarVisible=false; unresolvedField80=0;
    hudAtlas=s.loadAtlas(s.context,"data/textures/gui/HUDTEX.mpx",true,0,88,79,79);
    if (!hudAtlas || s.atlasTextureCount(s.context,hudAtlas)!=8) s.fatal(s.context);
    s.sounds.preload("swing1",2); s.sounds.preload("swing2",2); s.sounds.preload("goodhit",2);
    hudCenterX=s.screenWidth/2;
    auto* emitter=new Effect(3.0f); effect=emitter;
    emitter->setTexture("data/textures/effects/yellow.png");
    emitter->setPosition(static_cast<float>(hudCenterX)+207.0f,52,0,0);
    emitter->setRate(10); emitter->setDuration(50);
    emitter->setDirection(0,5,0); emitter->setDispersion(0.2f,0,0);
    emitter->setGravity(0,-0.5f,0); emitter->setVolume(0.6f,2,0);
    emitter->setScale(1); emitter->setFriction(1); emitter->setRamp(1);
    emitter->warmup(0); effectFrames=-1;
}
void Putter::setAnimation(int animation,PutterAnimationServices& s) {
    if (s.editing) return;
    animationId=animation;
    if (s.player) {
        if (animation<0) return;
        if (animation<10 && s.voiceSuffixes[animation]) {
            char name[20]; std::snprintf(name,sizeof(name),"%s%s",s.player->modelTag,s.voiceSuffixes[animation]);
            s.sounds.playVoice(name,0);
        }
    }
    if (animation==0) {
        s.player->shapeAdapter.setSequence(0,s.shape); s.player->shapeAdapter.setTime(0.5f,s.shape); s.player->shapeAdapter.playbackFlags&=~1u;
        conditionalPowerStep=0.5f; power=0; effectivePower=0; shotAngleOffset=0; inputGuardFrames=10; resetAimSmoothing=true;
        idleStart=s.tick; idleInterval=s.random.generate()%60u+300u;
        animationIdleStart=s.tick; animationIdleInterval=1800; gestureStart=s.tick; gestureInterval=0;
    } else if (animation>0 && animation<10) {
        s.player->shapeAdapter.setSequence(animation,s.shape); s.player->shapeAdapter.setTime(0,s.shape); s.player->shapeAdapter.startPlayback(s.shape);
        idleStart=s.tick; idleInterval=s.random.generate()%60u+300u;
    }
}
void Putter::computeShotVector(float speed,Point3F& output,const Player& player,BallsManager& balls) const {
    float heading=player.isHuman ? aimHeading : (player.ai.decisionState==4 ? player.ai.choice.heading : player.ai.smoothedHeading);
    float angle=heading*-0.017453292f;
    float x=static_cast<float>(std::cos(static_cast<long double>(angle)));
    float y=static_cast<float>(std::sin(static_cast<long double>(angle)));
    Ball* ball=player.getBall(balls);
    const float* normal=ball->restHistory[ball->restHistoryIndex].normal;
    float ax=-normal[2]*y,ay=normal[2]*x,az=normal[0]*y-normal[1]*x;
    output.x=ay*normal[2]-az*normal[1];
    output.y=az*normal[0]-normal[2]*ax;
    output.z=normal[1]*ax-ay*normal[0];
    float squared=output.x*output.x+output.y*output.y+output.z*output.z;
    long double length=std::sqrt(static_cast<long double>(squared));
    if (length!=0) {
        long double factor=static_cast<long double>(speed)/length;
        if (!player.isHuman) factor=static_cast<float>(factor);
        output.x=static_cast<float>(factor*output.x); output.y=static_cast<float>(factor*output.y); output.z=static_cast<float>(factor*output.z);
    }
}
void Putter::strikeBall(float speed,const PutterStrikeServices& s) {
    Point3F vector; computeShotVector(speed,vector,s.player,s.balls);
    if (shotAngleOffset!=0) {
        float angle=static_cast<float>(shotAngleOffset*3.1415927*0.0055555557);
        long double cosine=std::cos(static_cast<long double>(angle)),sine=std::sin(static_cast<long double>(angle));
        float x=static_cast<float>(static_cast<float>(cosine*vector.x)-sine*vector.y);
        float y=static_cast<float>(sine*vector.x+static_cast<float>(cosine*vector.y));
        vector.x=x; vector.y=y;
    }
    if (s.client && s.client->shouldWaitForRemoteTurn())
        vector={aimTarget[0],aimTarget[1],aimTarget[2]};
    if (s.client) s.client->sendPutt(power,vector);
    s.player.getBall(s.balls)->strike(vector,s.gameplay);
    s.camera.enterPostShotFollow(s.cameraRuntime,s.player,*s.player.getBall(s.balls));
}
void Putter::dispatchInput(PutterControlServices& s) {
    bool waitingForRemote=s.humanInput.client&&s.humanInput.client->shouldWaitForRemoteTurn();
    if (!s.players.isActive() || (s.hasTabElements && !waitingForRemote)) return;
    if (s.modifiers&1) switch (s.keyCode) {
    case '+': case '=': s.tuning.powerCurveOffset+=0.05; showPutterPowerCurve(s.tuning,s.players,s.fixedTick); break;
    case ',': case '<': --s.tuning.debugValue; break;
    case '-': case '_': s.tuning.powerCurveOffset-=0.05; showPutterPowerCurve(s.tuning,s.players,s.fixedTick); break;
    case '.': case '>': ++s.tuning.debugValue; break;
    case ':': case ';': s.player.controlMode=s.player.controlMode<2 ? s.player.controlMode+1 : 0; break;
    case '[': case '{': s.tuning.powerCurveQuadratic-=0.25; showPutterPowerCurve(s.tuning,s.players,s.fixedTick); break;
    case ']': case '}': s.tuning.powerCurveQuadratic+=0.25; showPutterPowerCurve(s.tuning,s.players,s.fixedTick); break;
    }
    if (!s.player.isHuman) handleAutomatedInput(s.automatedInput);
    else if (s.player.controlMode==0) handleInputMode0(s.humanInput);
    else if (s.player.controlMode==1) handleInputMode1(s.humanInput);
    else if (s.player.controlMode==2) handleInputMode2(s.humanInput);
}
void Putter::advanceControl(PutterControlServices& s) {
    float elapsed=0.033333335f;
    if (state==5 || state==6) {
        float value=power;
        if (value>=1) value=1; else if (value<=0.1) value=0.1f;
        elapsed=value*0.033333335f;
    }
    s.player.shapeAdapter.advanceTime(elapsed,s.shape);
    if (s.players.isActive()) {
        if (!s.player.isHuman) updateAutomatedStroke(s);
        else if (s.player.controlMode>=0 && s.player.controlMode<=2) {
            if (s.player.controlMode==0) updateMode0(s);
            else if (s.player.controlMode==1) updateMode1(s);
            else updateMode2(s);
            if (s.player.controlMode==0) {
                if(effect) static_cast<Effect*>(effect)->advanceParticles();
                if (effectFrames>=0) { if (!effectFrames && effect) static_cast<Effect*>(effect)->stop(); --effectFrames; }
            }
        }
    }
    s.guide.animateColors(s.fixedTick);
}
void Putter::updateAutomatedStroke(PutterControlServices& s) {
    if (s.player.isHuman) { if(s.playerServices.fatal)s.playerServices.fatal(s.playerServices.context); return; }
    auto& ai=s.player.ai;
    if (state==2) {
        float increment=static_cast<float>(1.0/(conditionalPowerStep*60.0));
        float scaled=power*0.1f;
        if (scaled<increment) increment=scaled<=0.1f ? 0.1f : scaled;
        power+=increment;
    }
    if (state==2 || state==4) {
        float value=ai.choice.power;
        if (power<value) value=power<=0 ? 0 : power;
        effectivePower=value;
        float time=static_cast<float>((1.0-power)*conditionalPowerStep);
        if (time>=conditionalPowerStep) time=conditionalPowerStep; else if (time<=0) time=0;
        s.player.shapeAdapter.setTime(time,s.shape); return;
    }
    if (state==3 || state==7 || state==8 || state==9) { if(s.playerServices.fatal)s.playerServices.fatal(s.playerServices.context); return; }
    if (state==5) {
        aimHeading=ai.decisionState==4 ? ai.choice.heading : ai.smoothedHeading;
        power=ai.choice.power;
        long double curve=evaluatePutterPowerCurve(power,s.tuning);
        strikeBall(static_cast<float>(-curve*shotSpeedScale),s.strike);
        state=6; ai.decisionState=5; return;
    }
    if (state==6 && (conditionalPowerStep<=s.player.shapeAdapter.getTime(s.shape) || !(s.player.shapeAdapter.playbackFlags&1))) {
        s.player.shapeAdapter.playbackFlags&=~1u; state=0; idleStart=s.fixedTick; idleInterval=0;
    }
}
void Putter::initialize(std::uint32_t tick) {
    idleStart=tick; idleInterval=0; animationIdleStart=tick; animationIdleInterval=1800;
    gestureStart=tick; gestureInterval=0; state=0; teeObject=nullptr; dragDeadZone=10; dragRange=80;
}
void Putter::initializePathSamples(PuttGuide& guide,std::uint32_t tick) {
    guide.buildSceneGeometry(10,tick); shotSpeedScale=1.5f;
}
void Putter::setTee(SceneObjectRecord& tee,BallsManager& balls,const BallRestServices& rest) {
    teeObject=&tee; balls.placeAllAtTee(tee,rest);
}
void Putter::forcePutt(float value,PutterAnimationServices& animation,const PutterStrikeServices& strike,
        const PutterTuning& tuning,float cameraHeading) {
    aimHeading=cameraHeading;
    setAnimation(0,animation); power=value;
    float time=static_cast<float>((1.0L-value)*conditionalPowerStep);
    if (time<conditionalPowerStep) { if (time<=0) time=0; }
    else time=conditionalPowerStep;
    animation.player->shapeAdapter.setTime(time,animation.shape);
    animation.player->shapeAdapter.startPlayback(animation.shape);
    const long double curve=evaluatePutterPowerCurve(value,tuning);
    strikeBall(static_cast<float>(-curve*shotSpeedScale),strike); state=6;
}
bool Putter::isTurnSettled(const Player& player,BallsManager& balls) const {
    if (state==1 && (player.getBall(balls)->flags&10)) return true;
    return state==0 && !(player.shapeAdapter.playbackFlags&1);
}
void Putter::captureInputAnchor(bool waitingForRemote,bool human,int mouseX,int mouseY) {
    if (!waitingForRemote && human) { inputAnchor[0]=static_cast<float>(mouseX); inputAnchor[1]=static_cast<float>(mouseY); }
    power=0;
}
void Putter::beginTurn(Player& player,Random& random,std::uint32_t tick,std::uint32_t taskTick,
                       PutterAnimationServices& animation,GameInputFrame& input,iRoomClient* client,
                       Camera& camera,CameraFrameServices& cameraFrame) {
    setAnimation(0,animation);
    captureInputAnchor(client&&client->shouldWaitForRemoteTurn(),player.isHuman,input.mouse.x,input.mouse.y);
    camera.resetFollowState(cameraFrame); player.ai.resetDecision(taskTick);
    state=1; inputGuardFrames=10; resetAimSmoothing=true;
    idleStart=tick; idleInterval=random.generate()%60u+300u;
    animationIdleStart=tick; animationIdleInterval=1800; gestureStart=tick; gestureInterval=0;
}
void Putter::setAimTargetWhileReady(const float* target) {
    if (state==1) for (int i=0;i<3;++i) aimTarget[i]=target[i];
}
void Putter::enterFollowThrough(float newPower,void* context,void (*enterCameraFollow)(void*)) {
    power=newPower; enterCameraFollow(context); conditionalPowerStep=0; state=6;
}
void Putter::handleAutomatedInput(PutterAutomatedServices& s) {
    Player* player=s.players.currentPlayer;
    if (!player || player->isHuman) { if(s.playerServices.fatal)s.playerServices.fatal(s.playerServices.context); return; }
    animationIdleStart=s.fixedTick; animationIdleInterval=1800;
    gestureStart=s.fixedTick; gestureInterval=0;
    Ball* ball=player->getBall(s.balls);
    if (!(ball->flags&1) || (ball->flags&10) || (ball->flags&0x20)) return;
    for (std::uint32_t i=0;i<s.players.playerCount;++i) {
        Player& other=s.players.playerStorage[i];
        if (!other.isDone(s.balls) && !(other.getBall(s.balls)->flags&1)) return;
    }
    if (s.client && s.client->shouldWaitForRemoteTurn()) {
        if (state==2 && power>=s.remotePowerThreshold) { state=5; player->shapeAdapter.startPlayback(s.shape); }
        return;
    }
    if (s.client && !s.client->isTurnStateReady()) return;
    if (state==9) { state=1; inputGuardFrames=10; return; }
    if (state==1) {
        if (!s.networkInGame && (s.inputButtons&0x01000000u) && s.cameraControlMode) s.camera.stopControlCam(s.cameraRuntime);
        if (player->ai.decisionState==1) {
            player->tryActivatePowerUp(s.playerServices.random,s.playerServices);
            setAnimation((s.animationRandom.generate()&1) ? 4 : 5,s.animation);
        }
        if (s.puttMap.updatePlayerDecision(*player,s.cameraControlMode,s.cameraHeading,s.taskTick)) {
            aimHeading=player->ai.decisionState==4 ? player->ai.choice.heading : player->ai.smoothedHeading;
            s.cameraHeading=aimHeading; s.camera.stopControlCam(s.cameraRuntime); setAnimation(0,s.animation);
            state=2; resetAimSmoothing=true; power=0;
            if (player->ai.choice.hasCondition) conditionalPowerStep=0.2f;
        }
    }
    if (state==2 && player->ai.choice.power<=power) state=4;
    if (state==4 && s.puttMap.testCondition(&player->ai.choice.condition,s.taskTick)) {
        state=5; player->shapeAdapter.startPlayback(s.shape);
    }
}
}
