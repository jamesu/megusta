#include "gameplay.h"
#include "runtime.h"
#include "game/player.h"
#include "game/balls.h"
#include "game/putt_map.h"
#include "game/ground_shadow.h"
#include "game/native_game_helpers.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
namespace megusta::bindings {
namespace { GameplayBindingServices* services=nullptr; }
void bindGameplayServices(GameplayBindingServices* value) {
    services=value;
    groundShadowCache.gl=value?&value->gl:nullptr;
    bindEditingState(value?&value->editorMode:nullptr);
}
GameplayBindingServices* gameplayServices() { return services; }
PyObject* missingGameplayServices() {
    PyErr_SetString(PyExc_RuntimeError,"Native gameplay binding services are not bound"); return nullptr;
}
namespace {
using Balls=BallsManager;
#include "wrapper_macros.h"
POINTER(Player,getBall,p->getBall(gameBalls),Ball)
INTEGER(Player,hasTakenMulligan,(p->flags>>3)&1u)
SIMPLE(Player,takeMulligan,NEED_GAMEPLAY p->takeMulligan(gamePlayers,gameBalls,services->players); return none())
SIMPLE(Player,giveBonus,if (p->strokeCount) --p->strokeCount; return none())
INTEGER(Player,isDone,p->isDone(gameBalls))
INTEGER(Player,getStroke,p->strokeCount)
INTEGER(Player,getStrokep,p->strokeCount+p->penaltyCount)
INTEGER(Player,getPlayClock,p->playClock)
SETINT(Player,setPlayClock,p->playClock=value)
SETINT(Player,setPoints,p->points=value)
INTEGER(Player,getPoints,p->points)
INTEGER(Player,getIndex,p->playerIndex)
TEXTUAL(Player,getName,p->name)
SETINT(Player,useMouse,p->useMouse=value!=0)
SWIG_BEGIN(Player,addPowerUp,"si",int value,&value)
    NEED_GAMEPLAY return Py_BuildValue(const_cast<char*>("i"),static_cast<int>(p->addPowerUp(static_cast<unsigned>(value),services->players))); }
SWIG_BEGIN(Player,getPowerUps,"si",int index,&index)
    return Py_BuildValue(const_cast<char*>("i"),static_cast<int>(p->powerUps[index].definitionId)); }
INTEGER(Player,getSelectedPowerUp,p->selectedPowerUpSlot)
SIMPLE(Player,usedPowerUp,NEED_GAMEPLAY
    services->lastUsedPowerUpId=p->powerUps[p->selectedPowerUpSlot].definitionId;
    p->powerUps[p->selectedPowerUpSlot].definitionId=0; p->powerUpActivationPending=false;
    p->usedPowerUp(services->players); return none())
SIMPLE(Player,incPenalty,p->lastStrokePenaltyFlag=true; ++p->penaltyCount; return none())
SIMPLE(Player,decPenalty,if (p->penaltyCount) --p->penaltyCount; return none())
SETINT(Player,setPutting,p->controlMode=value)
INTEGER(Player,getHandedness,p->leftHanded)
SIMPLE(Player,setLeftHanded,p->leftHanded=true; return none())
SIMPLE(Player,setRightHanded,p->leftHanded=false; return none())
INTEGER(Player,getIsHuman,p->isHuman)
SIMPLE(Player,setHuman,p->isHuman=true; return none())
SIMPLE(Player,setRobot,p->isHuman=false; return none())
SIMPLE(Player,setCPU,p->isHuman=false; return none())
SWIG_BEGIN(Player,setSkin,"si",int value,&value)
    NEED_GAMEPLAY p->setSkin(value,services->players); return none(); }
FLOATING(Player,getControl,p->controlAttribute)
FLOATING(Player,getAccuracy,p->accuracy)
FLOATING(Player,getLuck,p->luck)
SWIG_BEGIN(Player,setAttributes,"sfff",float control; float accuracy; float luck,&control,&accuracy,&luck)
    p->controlAttribute=control; p->accuracy=accuracy; p->luck=luck; return none(); }
INTEGER(Ball,onTee,p->restHistoryIndex==0)
INTEGER(Ball,isValid,p!=nullptr)
SIMPLE(Ball,getPlayer,NEED_GAMEPLAY return pointerResult(services->ball.playerByIndex(services->ball.context,p->playerIndex),"_Player_p"))
POINTER(Ball,getLoc,p->position,Point3F)
POINTER(Ball,getVel,p->velocity,Point3F)
#define VECTOR_SET(N,M) SWIG_BEGIN(Ball,N,"ss",char* encodedPoint,&encodedPoint) \
    auto* value=receiver<Point3F>(encodedPoint,"_Point3F_p","Ball_" #N); \
    if (!value && PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, \
        "Type error in argument 2 of Ball_" #N ". Expected _Point3F_p."); return nullptr; } \
    p->M(&value->x); return none(); }
VECTOR_SET(setLoc,setLocation)
VECTOR_SET(setVel,setVelocity)
#undef VECTOR_SET
SWIG_BEGIN(Ball,setColour,"si",int value,&value)
    p->colorModelIndex=value;
    if (value<0 || value>17) std::abort(); // Original fatal condition, not an added size guard.
    return none(); }
INTEGER(Ball,getColour,p->colorModelIndex)
INTEGER(Ball,isCloaked,(p->flags&10u)!=0)
SIMPLE(Ball,cloak,p->flags|=2u;
    for (auto* model:p->specialModel) model->clrTraversalMaskBits(1);
    return none())
SIMPLE(Ball,unCloak,p->flags&=~2u; return none())
INTEGER(Ball,inCup,(p->flags&0x10u)==0x10u)
SIMPLE(Ball,setInCup,p->setInCup(); return none())
SIMPLE(Ball,nearestCup,NEED_GAMEPLAY
    return Py_BuildValue(const_cast<char*>("f"),static_cast<double>(p->nearestCup(services->objects,services->puttMap))))
INTEGER(Ball,isResting,(p->flags&1u)==1u)
INTEGER(Ball,isMoving,(p->flags&0x2bu)==0)
INTEGER(Ball,isFrozen,(p->flags&0x20u)==0x20u)
INTEGER(Ball,IsRolling,p->isRolling())
SIMPLE(Ball,rest,NEED_GAMEPLAY p->rest(services->rest); return none())
SIMPLE(Ball,FindRestingLoc,NEED_GAMEPLAY p->findRestingLocation(services->rest); return none())
SIMPLE(Ball,lost,NEED_GAMEPLAY return Py_BuildValue(const_cast<char*>("i"),int(p->lost(services->ball))))
SETINT(Ball,setFrozen,if (value) p->flags|=0x20u; else p->flags&=~0x20u)
SETINT(Ball,setShadow,if (value) p->flags&=~0x40u; else p->flags|=0x40u)
SIMPLE(Ball,getLastTouch,NEED_GAMEPLAY return pointerResult(p->lastTouch(services->objects),"_Object_p"))
SETINT(Ball,setPowerUpFlag,p->turnFlag=static_cast<unsigned>(value))
SWIG_BEGIN(Ball,isPowerUpFlagSet,"si",int value,&value)
    return Py_BuildValue(const_cast<char*>("i"),int(p->turnFlag==static_cast<unsigned>(value))); }
INTEGER(Ball,isBallCollide,p->contact.kind==4)
POINTER(Ball,getLastB2B,p->contact.otherBall,Ball)
SWIG_BEGIN(Balls,b2b_flag_set,"si",int value,&value)
    p->collisionSeen=static_cast<unsigned>(value); return Py_BuildValue(const_cast<char*>("i"),value); }
INTEGER(Balls,b2b_flag_get,p->collisionSeen)
SWIG_BEGIN(Balls,get,"si",int index,&index) return pointerResult(p->balls+index,"_Ball_p"); }
POINTER(Balls,getActive,p->currentBall,Ball)
}
PyMethodDef* playerBallMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(Player_getBall) ENTRY(Player_hasTakenMulligan) ENTRY(Player_takeMulligan) ENTRY(Player_giveBonus)
        ENTRY(Player_isDone) ENTRY(Player_getStroke) ENTRY(Player_getStrokep) ENTRY(Player_getPlayClock)
        ENTRY(Player_setPlayClock) ENTRY(Player_setPoints) ENTRY(Player_getPoints) ENTRY(Player_getIndex)
        ENTRY(Player_getName) ENTRY(Player_useMouse) ENTRY(Player_addPowerUp) ENTRY(Player_getPowerUps)
        ENTRY(Player_getSelectedPowerUp) ENTRY(Player_usedPowerUp) ENTRY(Player_incPenalty) ENTRY(Player_decPenalty)
        ENTRY(Player_setPutting) ENTRY(Player_getHandedness) ENTRY(Player_setLeftHanded) ENTRY(Player_setRightHanded)
        ENTRY(Player_getIsHuman) ENTRY(Player_setHuman) ENTRY(Player_setRobot) ENTRY(Player_setCPU) ENTRY(Player_setSkin)
        ENTRY(Player_getControl) ENTRY(Player_getAccuracy) ENTRY(Player_getLuck) ENTRY(Player_setAttributes)
        ENTRY(Ball_onTee) ENTRY(Ball_isValid) ENTRY(Ball_getPlayer) ENTRY(Ball_getLoc) ENTRY(Ball_setLoc)
        ENTRY(Ball_getVel) ENTRY(Ball_setVel) ENTRY(Ball_setColour) ENTRY(Ball_getColour) ENTRY(Ball_isCloaked)
        ENTRY(Ball_cloak) ENTRY(Ball_unCloak) ENTRY(Ball_inCup) ENTRY(Ball_setInCup) ENTRY(Ball_nearestCup)
        ENTRY(Ball_isResting) ENTRY(Ball_isMoving) ENTRY(Ball_isFrozen) ENTRY(Ball_IsRolling) ENTRY(Ball_rest)
        ENTRY(Ball_FindRestingLoc) ENTRY(Ball_lost) ENTRY(Ball_setFrozen) ENTRY(Ball_setShadow) ENTRY(Ball_getLastTouch)
        ENTRY(Ball_setPowerUpFlag) ENTRY(Ball_isPowerUpFlagSet) ENTRY(Ball_isBallCollide) ENTRY(Ball_getLastB2B)
        ENTRY(Balls_b2b_flag_set) ENTRY(Balls_b2b_flag_get) ENTRY(Balls_get) ENTRY(Balls_getActive)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
