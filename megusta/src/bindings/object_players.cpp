#include "world.h"
#include "gameplay.h"
#include "runtime.h"
#include "game/scene_object.h"
#include "game/player.h"
#include "game/balls.h"
#include <cstdio>
namespace megusta::bindings {
namespace { SceneObjectAnimationServices* sceneAnimation=nullptr; }
void bindSceneAnimationServices(SceneObjectAnimationServices* value){sceneAnimation=value;}
SceneObjectAnimationServices* sceneAnimationServices(){return sceneAnimation?sceneAnimation:worldServices()?&worldServices()->animation:nullptr;}
namespace {
using Object=SceneObjectRecord;
using Objects=SceneObjectCollection;
using Players=PlayersManager;
#include "wrapper_macros.h"
#define NEED_WORLD if (!worldServices()) return missingWorldServices();
#define NEED_ANIMATION if(!sceneAnimationServices())return missingWorldServices();
template<class T> T* arg2(const char* value,const char* tag,const char* name,int number=2) {
    auto* result=receiver<T>(value,tag,name);
    if (!result && PyErr_Occurred()) { char message[256];
        std::sprintf(message,"Type error in argument %d of %s. Expected %s.",number,name,tag);
        PyErr_SetString(PyExc_TypeError,message); }
    return result;
}
SIMPLE(Object,isValid,NEED_GAMEPLAY return Py_BuildValue(const_cast<char*>("i"),int(p!=gameplayServices()->objects.sentinelObject)))
TEXTUAL(Object,getName,p->name)
SETINT(Object,setWire,NEED_WORLD p->setWire(value!=0,worldServices()->wireDebug))
SETINT(Object,setSync,if (value) p->flags|=0x40u; else p->flags&=~0x40u)
SIMPLE(Object,unCloak,NEED_GAMEPLAY NEED_ANIMATION p->uncloak(gameplayServices()->objects,*sceneAnimationServices()); return none())
SIMPLE(Object,cloak,NEED_GAMEPLAY NEED_ANIMATION p->cloak(gameplayServices()->objects,*sceneAnimationServices()); return none())
INTEGER(Object,isHidden,(p->flags&0x10u)==0x10u)
SIMPLE(Object,hide,p->hide(); return none())
SIMPLE(Object,show,p->show(); return none())
SWIG_BEGIN(Object,setAlpha,"sf",float value,&value) p->setAlpha(value); return none(); }
SETINT(Object,setDepthMask,setSceneRenderOrderMode(p->entity,value))
SWIG_BEGIN(Object,setSound,"ss",char* value,&value) p->setSound(value); return none(); }
SIMPLE(Object,setSeeThru,p->flags|=0x80u; return none())
// Original fixed 32-pointer friend lists at 0xe8, count at 0x168. Saved
// duplicate-check indices are inconsistent; typed iterator interpretation is
// explicitly comparison-pending rather than asserted instruction equivalence.
static void addFriend(Object& destination,Object* friendObject) {
    for (int i=0;i<destination.seeThruFriendCount;++i) if (destination.seeThruFriends[i]==friendObject) return;
    if (destination.seeThruFriendCount<32) destination.seeThruFriends[destination.seeThruFriendCount++]=friendObject;
}
SWIG_BEGIN(Object,addSeeThruFriend,"ss",char* value,&value)
    auto* other=arg2<Object>(value,"_Object_p","Object_addSeeThruFriend"); if (!other && PyErr_Occurred()) return nullptr;
    for (int i=0;i<p->seeThruFriendCount;++i) if (p->seeThruFriends[i]!=other) addFriend(*other,p->seeThruFriends[i]);
    for (int i=0;i<p->seeThruFriendCount;++i) if (p->seeThruFriends[i]!=other) addFriend(*p->seeThruFriends[i],other);
    if (p!=other) { addFriend(*other,p); addFriend(*p,other); } return none(); }
INTEGER(Object,getNumFrames,p->frameCount)
INTEGER(Object,getFrame,p->currentFrame)
SETINT(Object,setFrame,NEED_ANIMATION p->setFrame(static_cast<unsigned>(value),*sceneAnimationServices()))
SIMPLE(Object,move,NEED_ANIMATION p->move(*sceneAnimationServices()); return none())
SWIG_BEGIN(Object,setTrigger,"ss",char* value,&value)
    auto* task=arg2<Task>(value,"_Task_p","Object_setTrigger"); if (!task && PyErr_Occurred()) return nullptr;
    p->contactTask=task; return none(); }
SWIG_BEGIN(Object,selectKey,"ss",char* value,&value)
    auto* other=arg2<Object>(value,"_Object_p","Object_selectKey"); if (!other && PyErr_Occurred()) return nullptr;
    p->selectKey(*other); return none(); }
POINTER(Object,getPos,p->getPosition(),Point3F)
POINTER(Object,getDir,p->getDirection(),Point3F)
FLOATING(Object,getBallDist,p->getBallDistance(gamePlayers.currentPlayer->getBall(gameBalls)->position))
SWIG_BEGIN(Object,setCOR,"sf",float value,&value) p->coefficientOfRestitution=value; return none(); }
SWIG_BEGIN(Object,setCOF,"sf",float value,&value) p->coefficientOfFriction=value; return none(); }
SWIG_BEGIN(Object,find,"ss",char* name,&name)
    NEED_GAMEPLAY return pointerResult(gameplayServices()->objects.findWithinRoot(p->entity,name),"_Object_p"); }
INTEGER(Objects,getCount,p->objectCount)
SWIG_BEGIN(Objects,get,"si",int index,&index) return pointerResult(p->objects+index,"_Object_p"); }
SWIG_BEGIN(Objects,find,"ss",char* name,&name) return pointerResult(p->findByName(name),"_Object_p"); }
SWIG_BEGIN(Objects,setDefaultSound,"ss",char* name,&name)
    for (int i=1;i<=p->objectCount;++i) {
        auto& object=p->objects[i];
        if (object.name && !(object.flags&1u) && !object.impactSound && !object.contactTask) object.setSound(name);
    }
    return none(); }
SIMPLE(Players,reset,p->reset(); return none())
SWIG_BEGIN(Players,add,"sss",char* name; char* model,&name,&model)
    NEED_GAMEPLAY return pointerResult(p->add(name,model,gameplayServices()->players),"_Player_p"); }
SWIG_BEGIN(Players,get,"si",int index,&index)
    NEED_GAMEPLAY return pointerResult(p->get(static_cast<unsigned>(index),gameplayServices()->players),"_Player_p"); }
INTEGER(Players,getCount,p->playerCount)
POINTER(Players,getCurrent,p->currentPlayer,Player)
SWIG_BEGIN(Players,setCurrent,"ss",char* value,&value)
    auto* player=arg2<Player>(value,"_Player_p","Players_setCurrent"); if (!player && PyErr_Occurred()) return nullptr;
    NEED_GAMEPLAY p->setCurrent(player,gameBalls,gameplayServices()->players); return none(); }
SWIG_BEGIN(Players,findNext,"ss",char* value,&value)
    auto* player=arg2<Player>(value,"_Player_p","Players_findNext"); if (!player && PyErr_Occurred()) return nullptr;
    return pointerResult(p->findNext(player),"_Player_p"); }
SWIG_BEGIN(Players,setEventHandler,"sis",int slot; char* value,&slot,&value)
    auto* task=arg2<Task>(value,"_Task_p","Players_setEventHandler",3); if (!task && PyErr_Occurred()) return nullptr;
    p->setEventHandler(slot,task); return none(); }
INTEGER(Players,getDisplayStroke,p->displayedStrokeCount)
SWIG_BEGIN(Players,showMessage,"ssi",char* value; int duration,&value,&duration)
    NEED_GAMEPLAY const auto& s=gameplayServices()->players;
    gamePlayers.showMessage(value,static_cast<unsigned>(duration),s.timeMilliseconds(s.context)); return none(); }
INTEGER(Players,isTurnEnded,p->turnState==4)
SETINT(Players,setGlobalHandedness,p->holeFlag=value!=0)
}
PyMethodDef* objectPlayerMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(Object_isValid) ENTRY(Object_getName) ENTRY(Object_setWire) ENTRY(Object_setSync)
        ENTRY(Object_unCloak) ENTRY(Object_cloak) ENTRY(Object_isHidden) ENTRY(Object_hide) ENTRY(Object_show)
        ENTRY(Object_setAlpha) ENTRY(Object_setDepthMask) ENTRY(Object_setSound) ENTRY(Object_setSeeThru)
        ENTRY(Object_addSeeThruFriend) ENTRY(Object_getNumFrames) ENTRY(Object_getFrame) ENTRY(Object_setFrame)
        ENTRY(Object_move) ENTRY(Object_setTrigger) ENTRY(Object_selectKey) ENTRY(Object_getPos) ENTRY(Object_getDir)
        ENTRY(Object_getBallDist) ENTRY(Object_setCOR) ENTRY(Object_setCOF) ENTRY(Object_find)
        ENTRY(Objects_getCount) ENTRY(Objects_get) ENTRY(Objects_find) ENTRY(Objects_setDefaultSound)
        ENTRY(Players_reset) ENTRY(Players_add) ENTRY(Players_get) ENTRY(Players_getCount) ENTRY(Players_getCurrent)
        ENTRY(Players_setCurrent) ENTRY(Players_findNext) ENTRY(Players_setEventHandler) ENTRY(Players_getDisplayStroke)
        ENTRY(Players_showMessage) ENTRY(Players_isTurnEnded) ENTRY(Players_setGlobalHandedness)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
