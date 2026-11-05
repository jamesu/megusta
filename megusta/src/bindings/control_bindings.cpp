#include "world.h"
#include "gameplay.h"
#include "runtime.h"
#include "game/camera.h"
#include "game/putter.h"
#include "game/player.h"
#include "game/driver.h"
#include "game/native_game_helpers.h"
#include <new>
#include <cstdio>
#include <memory>
namespace megusta::bindings {
namespace { CameraBindingServices* cameraBindings=nullptr;bool explicitCamera=false;std::unique_ptr<CameraBindingServices> legacyCamera; }
void bindCameraServices(CameraBindingServices* value){explicitCamera=value!=nullptr;cameraBindings=value?value:legacyCamera.get();}
namespace { WorldBindingServices* services=nullptr; DriverScriptServices* driverScripts=nullptr; void (*driverJoystick)(bool)=nullptr; }
void bindDriverServices(DriverScriptServices* scripts,void (*joystick)(bool)) { driverScripts=scripts;driverJoystick=joystick; }
HoleServices* boundHoleServices(){return driverScripts?&driverScripts->holeServices:nullptr;}
void bindWorldServices(WorldBindingServices* value) {
    services=value;bindDemoState(value?&value->demo:nullptr);
    legacyCamera.reset(value?new CameraBindingServices{value->camera,value->fixedTick,value->collisionFilter,value->showCollisionMessage}:nullptr);
    if(!explicitCamera)cameraBindings=legacyCamera.get();
}
WorldBindingServices* worldServices() { return services; }
PyObject* missingWorldServices() {
    PyErr_SetString(PyExc_RuntimeError,"Native world binding services are not bound"); return nullptr;
}
namespace {
#include "wrapper_macros.h"
#define NEED_WORLD if (!services) return missingWorldServices();
#define NEED_CAMERA if(!cameraBindings)return missingWorldServices();
static bool editing() { return gameplayServices()->editorMode!=0; }
SIMPLE(Camera,allowCollision,NEED_CAMERA Camera::allowCollision(cameraBindings->collisionFilter,cameraBindings->showCollisionMessage); return none())
SIMPLE(Camera,disallowCollision,NEED_CAMERA Camera::disallowCollision(cameraBindings->collisionFilter); return none())
INTEGER(Camera,collisionEnabled,Camera::collisionEnabled())
SETINT(Camera,setState,NEED_CAMERA p->setState(cameraBindings->state,static_cast<CameraMode>(value)))
FLOATING(Camera,getHeading,p->headingDegrees)
SWIG_BEGIN(Camera,setHeading,"sf",float value,&value) p->savedHeadingDegrees=value; p->headingDegrees=value; return none(); }
SWIG_BEGIN(Camera,setPitch,"sf",float value,&value)
    NEED_GAMEPLAY p->pitchDegrees=value; p->clampEditorOrbit(editing); return none(); }
SWIG_BEGIN(Camera,setFOV,"sf",float value,&value)
    if (value<180) { if (value<=10) value=10; } else value=180;
    p->fieldOfView=value; ssgGetCurrentContext()->setFOV(value,0); return none(); }
SWIG_BEGIN(Camera,setPosition,"sfff",float x; float y; float z,&x,&y,&z)
    NEED_CAMERA p->setPosition(cameraBindings->state,cameraBindings->fixedTick,x,y,z); return none(); }
SWIG_BEGIN(Camera,setLookat,"sfff|f",float x; float y; float z; float seconds=2.5f,&x,&y,&z,&seconds)
    NEED_CAMERA p->setLookat(cameraBindings->state,cameraBindings->fixedTick,x,y,z,seconds); return none(); }
SWIG_BEGIN(Camera,adjustEyeDist,"sf",float value,&value)
    NEED_GAMEPLAY p->followDistance+=value; p->clampEditorOrbit(editing); return none(); }
SWIG_BEGIN(Camera,setEyeDist,"sf",float value,&value) p->followDistance=value; return none(); }
SIMPLE(Camera,setLookatPlayer,NEED_CAMERA p->setLookatPlayer(cameraBindings->state); return none())
SIMPLE(Camera,setLookatBall,NEED_CAMERA p->setLookatBall(cameraBindings->state); return none())
SIMPLE(Camera,setDefault,NEED_CAMERA p->restartFollow(cameraBindings->state); return none())
SIMPLE(Camera,stopControlCam,NEED_CAMERA return Py_BuildValue(const_cast<char*>("i"),int(p->stopControlCam(cameraBindings->state))))
SWIG_BEGIN(Camera,loadControlPath,"ss",char* name,&name)
    NEED_CAMERA NEED_GAMEPLAY if(!sceneAnimationServices())return missingWorldServices();return Py_BuildValue(const_cast<char*>("i"),
        int(Camera::loadControlPath(cameraBindings->state,gameplayServices()->objects,name,*sceneAnimationServices()))); }
SWIG_BEGIN(Camera,addControlPoint,"sfff",float x; float y; float z,&x,&y,&z)
    NEED_CAMERA Camera::addControlPoint(cameraBindings->state,x,y,z); return none(); }
SWIG_BEGIN(Camera,addControlPoint2,"ss",char* encodedPoint,&encodedPoint)
    auto* value=receiver<Point3F>(encodedPoint,"_Point3F_p","Camera_addControlPoint2");
    if (!value && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,"Type error in argument 2 of Camera_addControlPoint2. Expected _Point3F_p."); return nullptr;
    }
    NEED_CAMERA Camera::addControlPoint(cameraBindings->state,*value); return none(); }
SWIG_BEGIN(Camera,startControlCam,"sf",float value,&value)
    NEED_CAMERA p->startControlCam(cameraBindings->state,value); return none(); }
INTEGER(Camera,checkControlCam,p->checkControlCam())
SWIG_BEGIN(Camera,jitter,"sff",float ignored; float seconds,&ignored,&seconds)
    NEED_CAMERA Camera::jitter(cameraBindings->state,cameraBindings->fixedTick,seconds); return none(); }
SIMPLE(Camera,getTarget,return pointerResult(new(std::nothrow) Point3F(p->controlLookAt),"_Point3F_p"))
SIMPLE(Camera,getLoc,return pointerResult(new(std::nothrow) Point3F(p->controlEye),"_Point3F_p"))
SIMPLE(Camera,dump,NEED_WORLD if (!services->cameraDump) return missingWorldServices(); services->cameraDump(*p); return none())
POINTER(Putter,getTee,p->teeObject,Object)
SWIG_BEGIN(Putter,setTee,"ss",char* encodedObject,&encodedObject)
    auto* value=receiver<SceneObjectRecord>(encodedObject,"_Object_p","Putter_setTee");
    if (!value && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,"Type error in argument 2 of Putter_setTee. Expected _Object_p."); return nullptr;
    }
    NEED_GAMEPLAY p->setTee(*value,gameBalls,gameplayServices()->rest); return none(); }
SETINT(Putter,setAnim,NEED_WORLD
    auto& animation=services->putterAnimation;
    animation.player=gamePlayers.currentPlayer; animation.tick=services->fixedTick; animation.editing=editing();
    if (!animation.player) return missingWorldServices();
    p->setAnimation(value,animation))
FLOATING(Putter,getPower,p->power)
SWIG_BEGIN(Putter,forcePutt,"sf",float value,&value)
    NEED_WORLD
    if (!gamePlayers.currentPlayer) return missingWorldServices();
    auto& animation=services->putterAnimation;
    animation.player=gamePlayers.currentPlayer; animation.tick=services->fixedTick; animation.editing=editing();
    const auto& bound=services->strike;
    PutterStrikeServices strike{*gamePlayers.currentPlayer,gameBalls,gameplayServices()->ball,bound.context,
        bound.waitForRemoteTurn,bound.remoteVector,bound.sendPutt,bound.enterCameraFollow};
    p->forcePutt(value,animation,strike,services->tuning,services->cameraHeading); return none(); }
SETINT(Putter,showPowerBar,p->powerBarVisible=value!=0)
INTEGER(Putter,isAiming,p->state==1)
INTEGER(Putter,isSwinging,p->state==2 || p->state==3)
INTEGER(Putter,isPutting,1)
POINTER(Putter,getEffect,p->effect,Effect)
SIMPLE(Putter,dump,p->dump(); return none())
SWIG_BEGIN(Driver,changeScript,"ss",char* name,&name)
    auto* scripts=driverScripts?driverScripts:(services?&services->scripts:nullptr);
    if(!scripts)return missingWorldServices(); p->changeScript(name,*scripts); return none(); }
SIMPLE(Driver,reloadScript,auto* scripts=driverScripts?driverScripts:(services?&services->scripts:nullptr); if(!scripts)return missingWorldServices(); p->reloadScript(*scripts); return none())
TEXTUAL(Driver,getScriptName,p->activeScript)
SETINT(Driver,useJoystick,auto callback=driverJoystick?driverJoystick:(services?services->registerJoystick:nullptr); if(!callback)return missingWorldServices(); callback(value!=0))
SETINT(Driver,showFPS,p->fpsOverlayEnabled=(value&1)!=0; p->frameLimitEnabled=((static_cast<unsigned>(value)>>1)&1u)==0)
SIMPLE(Driver,isDemo,return Py_BuildValue(const_cast<char*>("i"),int(isDemoMode())))
}
PyMethodDef* cameraPutterDriverMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(Camera_allowCollision) ENTRY(Camera_disallowCollision) ENTRY(Camera_collisionEnabled) ENTRY(Camera_setState)
        ENTRY(Camera_getHeading) ENTRY(Camera_setHeading) ENTRY(Camera_setPitch) ENTRY(Camera_setFOV)
        ENTRY(Camera_setPosition) ENTRY(Camera_setLookat) ENTRY(Camera_adjustEyeDist) ENTRY(Camera_setEyeDist)
        ENTRY(Camera_setLookatPlayer) ENTRY(Camera_setLookatBall) ENTRY(Camera_setDefault) ENTRY(Camera_stopControlCam)
        ENTRY(Camera_loadControlPath) ENTRY(Camera_addControlPoint) ENTRY(Camera_addControlPoint2) ENTRY(Camera_startControlCam)
        ENTRY(Camera_checkControlCam) ENTRY(Camera_jitter) ENTRY(Camera_getTarget) ENTRY(Camera_getLoc) ENTRY(Camera_dump)
        ENTRY(Putter_getTee) ENTRY(Putter_setTee) ENTRY(Putter_setAnim) ENTRY(Putter_getPower) ENTRY(Putter_forcePutt)
        ENTRY(Putter_showPowerBar) ENTRY(Putter_isAiming) ENTRY(Putter_isSwinging) ENTRY(Putter_isPutting) ENTRY(Putter_getEffect) ENTRY(Putter_dump)
        ENTRY(Driver_changeScript) ENTRY(Driver_reloadScript) ENTRY(Driver_getScriptName) ENTRY(Driver_useJoystick) ENTRY(Driver_showFPS) ENTRY(Driver_isDemo)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
