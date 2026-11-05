#include "remaining_game_bindings.h"
#include "runtime.h"
#include "game/powerups.h"
#include "game/task.h"
#include "ssg.h"
#include "ssgGameAnimation.h"
#include "sg.h"
#include "game/scene_triangle.h"
#include "game/gui.h"
#include "pu.h"
#include "game/input.h"
#include "game/driver.h"
#include "game/hole.h"
#include "game/game_shape_adapter.h"
#include "game/scene_object_literals.h"
#include "game/gui_assets.h"
#include "game/scene_runtime.h"
#include "game/gui_runtime.h"
#include "game/gui_zoom.h"
#include "engine/opengl_adapter.h"
#include "gameplay.h"
#include "world.h"
#include "game/player.h"
#include "game/balls.h"
#include "engine/string_services.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <exception>
namespace megusta::bindings {
namespace {
bool checkPointer(const char* encoded,const char* tag,const char* function,int number) {
    if (!encoded) return true;
    decodeReceiver(encoded,tag,function);
    if (!PyErr_Occurred()) return true;
    PyErr_Clear();
    char message[256];
    std::sprintf(message,"Type error in argument %d of %s. Expected %s.",number,function,tag);
    PyErr_SetString(PyExc_TypeError,message);
    return false;
}
PyObject* unsupportedNativeOperation(const char* message) {
    PyErr_SetString(PyExc_RuntimeError,message); return nullptr;
}
using SceneBindingState=SceneRoots;
static PyObject* Scene_root_set(PyObject*,PyObject* args) {
    char *encoded,*rootText;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ss:Scene_root_set"),&encoded,&rootText)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_root_set"); if(!scene&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(rootText,"_ssgRoot_p","Scene_root_set",2)) return nullptr;
    scene->root=static_cast<ssgRoot*>(decodeReceiver(rootText,"_ssgRoot_p","Scene_root_set"));
    return pointerResult(scene->root,"_ssgRoot_p");
}
static PyObject* Scene_root_get(PyObject*,PyObject* args) {
    char* encoded; if(!PyArg_ParseTuple(args,const_cast<char*>("s:Scene_root_get"),&encoded)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_root_get"); if(!scene&&PyErr_Occurred()) return nullptr;
    return pointerResult(scene->root,"_ssgRoot_p");
}
static PyObject* Scene_load(PyObject*,PyObject* args) {
    char* encoded; char* filename; int textureEnabled=1;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ss|i:Scene_load"),&encoded,&filename,&textureEnabled)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_load"); if(!scene&&PyErr_Occurred()) return nullptr;
    (void)scene;
    return pointerResult(loadSceneModel(filename,textureEnabled!=0),"_ssgEntity_p");
}
static PyObject* Scene_createTransform(PyObject*,PyObject* args) {
    char* encoded; float x,y,z,angle;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sffff:Scene_createTransform"),&encoded,&x,&y,&z,&angle)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_createTransform"); if(!scene&&PyErr_Occurred()) return nullptr;
    (void)scene;
    return pointerResult(createSceneTransform(x,y,z,angle),"_ssgTransform_p");
}
static PyObject* Scene_createAnim(PyObject*,PyObject* args) {
    char *encoded,*entityText;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ss:Scene_createAnim"),&encoded,&entityText)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_createAnim"); if(!scene&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(entityText,"_ssgEntity_p","Scene_createAnim",2)) return nullptr;
    auto* entity=static_cast<ssgEntity*>(decodeReceiver(entityText,"_ssgEntity_p","Scene_createAnim"));
    return pointerResult(createSceneAnimation(entity),"_ssgAnim_p");
}
static PyObject* Scene_hideFakeObjects(PyObject*,PyObject* args) {
    char *encoded,*entityText;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ss:Scene_hideFakeObjects"),&encoded,&entityText)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_hideFakeObjects"); if(!scene&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(entityText,"_ssgEntity_p","Scene_hideFakeObjects",2)) return nullptr;
    auto* entity=static_cast<ssgEntity*>(decodeReceiver(entityText,"_ssgEntity_p","Scene_hideFakeObjects"));
    hideFakeSceneObjects(entity);
    return none();
}
static PyObject* Scene_hide(PyObject*,PyObject* args) {
    char *encoded,*entityText; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:Scene_hide"),&encoded,&entityText)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_hide"); if(!scene&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(entityText,"_ssgEntity_p","Scene_hide",2)) return nullptr;
    auto* entity=static_cast<ssgEntity*>(decodeReceiver(entityText,"_ssgEntity_p","Scene_hide"));
    hideSceneEntity(entity); return none();
}
static PyObject* Scene_show(PyObject*,PyObject* args) {
    char *encoded,*entityText; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:Scene_show"),&encoded,&entityText)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_show"); if(!scene&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(entityText,"_ssgEntity_p","Scene_show",2)) return nullptr;
    auto* entity=static_cast<ssgEntity*>(decodeReceiver(entityText,"_ssgEntity_p","Scene_show"));
    showSceneEntity(entity); return none();
}
static PyObject* Scene_setCamera(PyObject*,PyObject* args) {
    char *encoded,*eyeText,*targetText;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sss:Scene_setCamera"),&encoded,&eyeText,&targetText)) return nullptr;
    auto* scene=receiver<SceneBindingState>(encoded,"_Scene_p","Scene_setCamera"); if(!scene&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(eyeText,"_Point3F_p","Scene_setCamera",2)||!checkPointer(targetText,"_Point3F_p","Scene_setCamera",3)) return nullptr;
    auto* eye=static_cast<Point3F*>(decodeReceiver(eyeText,"_Point3F_p","Scene_setCamera"));
    auto* target=static_cast<Point3F*>(decodeReceiver(targetText,"_Point3F_p","Scene_setCamera"));
    setSceneCamera(&eye->x,&target->x);
    return none();
}

static PyObject* guiElement_setWriteOnlyAttr(PyObject*,PyObject* args) {
    char* encoded; int which; char* value;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sis:guiElement_setWriteOnlyAttr"),&encoded,&which,&value)) return nullptr;
    auto* element=receiver<guiElement>(encoded,"_guiElement_p","guiElement_setWriteOnlyAttr"); if(!element&&PyErr_Occurred()) return nullptr;
    element->setWriteOnlyAttr(which,value);
    return none();
}
static PyObject* guiElement_specialCmd(PyObject*,PyObject* args) {
    char *encoded,*command,*value; int a,b,c,d;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sssiiii:guiElement_specialCmd"),&encoded,&command,&value,&a,&b,&c,&d)) return nullptr;
    auto* element=receiver<guiElement>(encoded,"_guiElement_p","guiElement_specialCmd"); if(!element&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),element->specialCmd(command,value,a,b,c,d));
}
static PyObject* guiBindings_add(PyObject*,PyObject* args) {
    char *encoded,*key,*action; if(!PyArg_ParseTuple(args,const_cast<char*>("sss:guiBindings_add"),&encoded,&key,&action)) return nullptr;
    auto* bindings=receiver<guiBindings>(encoded,"_guiBindings_p","guiBindings_add"); if(!bindings&&PyErr_Occurred()) return nullptr;
    bindings->add(key,action);
    return none();
}
static PyObject* guiBindings_remove(PyObject*,PyObject* args) {
    char *encoded,*key; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:guiBindings_remove"),&encoded,&key)) return nullptr;
    auto* bindings=receiver<guiBindings>(encoded,"_guiBindings_p","guiBindings_remove"); if(!bindings&&PyErr_Occurred()) return nullptr;
    bindings->remove(key);
    return none();
}
static PyObject* guiSystem_setBackground(PyObject*,PyObject* args) {
    char *encoded,*path; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:guiSystem_setBackground"),&encoded,&path)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_setBackground"); if(!system&&PyErr_Occurred()) return nullptr;
    system->setBackground(*path?path:nullptr);
    return none();
}
static PyObject* guiSystem_addElement(PyObject*,PyObject* args) {
    char* encoded; int type,x,y,width,height; int o0=0,o1=0,o2=0,o3=0;
    if(!PyArg_ParseTuple(args,const_cast<char*>("siiiii|iiii:guiSystem_addElement"),&encoded,&type,&x,&y,&width,&height,&o0,&o1,&o2,&o3)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_addElement"); if(!system&&PyErr_Occurred()) return nullptr;
    auto* element=system->addElement(type,x,y,width,height,static_cast<unsigned>(o0),static_cast<unsigned>(o1),static_cast<unsigned>(o2),o3);
    if(!element) return unsupportedNativeOperation("guiSystem_addElement: game widget type is not implemented");
    return pointerResult(element,"_guiElement_p");
}
static PyObject* guiSystem_removeElement(PyObject*,PyObject* args) {
    char *encoded,*elementText; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:guiSystem_removeElement"),&encoded,&elementText)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_removeElement"); if(!system&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(elementText,"_guiElement_p","guiSystem_removeElement",2)) return nullptr;
    system->removeElement(static_cast<guiElement*>(decodeReceiver(elementText,"_guiElement_p","guiSystem_removeElement")));
    return none();
}
static PyObject* guiSystem_setCurrentTab(PyObject*,PyObject* args) {
    char *encoded,*elementText; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:guiSystem_setCurrentTab"),&encoded,&elementText)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_setCurrentTab"); if(!system&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(elementText,"_guiElement_p","guiSystem_setCurrentTab",2)) return nullptr;
    system->setCurrentTab(static_cast<guiElement*>(decodeReceiver(elementText,"_guiElement_p","guiSystem_setCurrentTab")),true);
    return none();
}
static PyObject* guiSystem_findNearestTab(PyObject*,PyObject* args) {
    char* encoded; int dx,dy; if(!PyArg_ParseTuple(args,const_cast<char*>("sii:guiSystem_findNearestTab"),&encoded,&dx,&dy)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_findNearestTab"); if(!system&&PyErr_Occurred()) return nullptr;
    return pointerResult(system->findNearestTab(dx,dy),"_guiElement_p");
}
static PyObject* guiSystem_findMouseOver(PyObject*,PyObject* args) {
    char* encoded; if(!PyArg_ParseTuple(args,const_cast<char*>("s:guiSystem_findMouseOver"),&encoded)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_findMouseOver"); if(!system&&PyErr_Occurred()) return nullptr;
    return pointerResult(system->findMouseOver(gameInput.mouse.x,gameInput.mouse.y,driverViewport.displayHeight),"_guiElement_p");
}
static PyObject* guiSystem_shellCmd(PyObject*,PyObject* args) {
    char *encoded,*command; int a=0,b=0,c=0,d=0,e=0;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ss|iiiii:guiSystem_shellCmd"),&encoded,&command,&a,&b,&c,&d,&e)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_shellCmd"); if(!system&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),system->shellCmd(command,a,b,c,d,e));
}
static PyObject* guiSystem_dtsSetSkin(PyObject*,PyObject* args) {
    char *encoded,*name; int skin;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ssi:guiSystem_dtsSetSkin"),&encoded,&name,&skin)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_dtsSetSkin"); if(!system&&PyErr_Occurred()) return nullptr;
    system->setShapeSkin(name,skin); return none();
}
static PyObject* guiSystem_dtsLoad(PyObject*,PyObject* args) {
    char *encoded,*path; int slot; unsigned flags; int textureEnabled; int unused=0;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sisii|i:guiSystem_dtsLoad"),&encoded,&slot,&path,&flags,&textureEnabled,&unused)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_dtsLoad"); if(!system&&PyErr_Occurred()) return nullptr;
    if(slot<0||slot>=4) { PyErr_SetString(PyExc_ValueError,"guiSystem_dtsLoad preview slot must be 0..3"); return nullptr; }
    system->loadShapePreview(slot,path,flags,static_cast<unsigned>(textureEnabled)); return none();
}
static PyObject* guiSystem_dtsCmd(PyObject*,PyObject* args) {
    char *encoded,*command; int slot,a=0,b=0,c=0,d=0;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sis|iiii:guiSystem_dtsCmd"),&encoded,&slot,&command,&a,&b,&c,&d)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_dtsCmd"); if(!system&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),system->shapeCommand(slot,command,a,b,c,d));
}
static PyObject* guiSystem_dtsFlush(PyObject*,PyObject* args) {
    char* encoded; if(!PyArg_ParseTuple(args,const_cast<char*>("s:guiSystem_dtsFlush"),&encoded)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_dtsFlush"); if(!system&&PyErr_Occurred()) return nullptr;
    system->flushShapePreviews(); return none();
}
static PyObject* guiSystem_getStringWidth(PyObject*,PyObject* args) {
    char *encoded,*value; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:guiSystem_getStringWidth"),&encoded,&value)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_getStringWidth"); if(!system&&PyErr_Occurred()) return nullptr;
    (void)system; return Py_BuildValue(const_cast<char*>("i"),puGetStringWidth(puGetDefaultLegendFont(),value));
}
static PyObject* guiSystem_getStringHeight(PyObject*,PyObject* args) {
    char *encoded,*value; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:guiSystem_getStringHeight"),&encoded,&value)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_getStringHeight"); if(!system&&PyErr_Occurred()) return nullptr;
    (void)system; return Py_BuildValue(const_cast<char*>("i"),puGetStringHeight(puGetDefaultLegendFont(),value));
}
static PyObject* guiSystem_preloadZoom(PyObject*,PyObject* args) {
    char *encoded,*name,*filename,*material0,*material1;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sssss:guiSystem_preloadZoom"),&encoded,&name,&filename,&material0,&material1)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_preloadZoom"); if(!system&&PyErr_Occurred()) return nullptr;
    preloadGuiZoom(name,filename,material0,material1); return none();
}
static PyObject* guiSystem_playZoom(PyObject*,PyObject* args) {
    char *encoded,*name; int holdTicks;
    if(!PyArg_ParseTuple(args,const_cast<char*>("ssi:guiSystem_playZoom"),&encoded,&name,&holdTicks)) return nullptr;
    auto* system=receiver<guiSystem>(encoded,"_guiSystem_p","guiSystem_playZoom"); if(!system&&PyErr_Occurred()) return nullptr;
    system->playZoom(name,static_cast<std::uint32_t>(holdTicks));
    return none();
}

static PyObject* Hole_init(PyObject*,PyObject* args) {
    char *encoded,*first,*second; int par;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sssi:Hole_init"),&encoded,&first,&second,&par)) return nullptr;
    auto* hole=receiver<Hole>(encoded,"_Hole_p","Hole_init"); if(!hole&&PyErr_Occurred()) return nullptr;
    auto* services=boundHoleServices();if(!services)return missingGameplayServices();
    hole->initialize(first,second,par,gameHoleClock,*services);
    return none();
}
static PyObject* Hole_getName(PyObject*,PyObject* args) {
    char* encoded; if(!PyArg_ParseTuple(args,const_cast<char*>("s:Hole_getName"),&encoded)) return nullptr;
    auto* hole=receiver<Hole>(encoded,"_Hole_p","Hole_getName"); if(!hole&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("s"),hole->ownedInitString1);
}
static PyObject* Hole_getPar(PyObject*,PyObject* args) {
    char* encoded; if(!PyArg_ParseTuple(args,const_cast<char*>("s:Hole_getPar"),&encoded)) return nullptr;
    auto* hole=receiver<Hole>(encoded,"_Hole_p","Hole_getPar"); if(!hole&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),hole->initIntegerArgument);
}
static PyObject* Hole_setAmbientColour(PyObject*,PyObject* args) {
    char* encoded; float red,green,blue;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sfff:Hole_setAmbientColour"),&encoded,&red,&green,&blue)) return nullptr;
    auto* hole=receiver<Hole>(encoded,"_Hole_p","Hole_setAmbientColour"); if(!hole&&PyErr_Occurred()) return nullptr;
    (void)hole; setGameShapeMeshColour(red,green,blue); return none();
}
static PyObject* AI_cmd(PyObject*,PyObject* args) {
    char *encoded,*command; if(!PyArg_ParseTuple(args,const_cast<char*>("ss:AI_cmd"),&encoded,&command)) return nullptr;
    (void)encoded;
    auto* services=gameplayServices();
    if(!services) { PyErr_SetString(PyExc_RuntimeError,"AI command requires bound gameplay services"); return nullptr; }
    try {
        const int result=executeAICommand(command,gamePlayers,gameBalls,services->players,services->scriptRuntime,
            gameHole.initIntegerArgument,gameTasks.tickCount);
        return Py_BuildValue(const_cast<char*>("i"),result);
    } catch (const std::exception& error) { PyErr_SetString(PyExc_RuntimeError,error.what()); return nullptr; }
}
static PyObject* PowerUps_addHoleCoin(PyObject*,PyObject* args) {
    char* encoded; int id; float x,y,z,radius;
    if(!PyArg_ParseTuple(args,const_cast<char*>("siffff:PowerUps_addHoleCoin"),&encoded,&id,&x,&y,&z,&radius)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_addHoleCoin"); if(!p&&PyErr_Occurred()) return nullptr;
    p->addHoleCoin(id,x,y,z,radius); return none();
}
static PyObject* PowerUps_deployCoins(PyObject*,PyObject* args) {
    char* encoded; int first,second=0,third=0,fourth=0;
    if(!PyArg_ParseTuple(args,const_cast<char*>("si|iii:PowerUps_deployCoins"),&encoded,&first,&second,&third,&fourth)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_deployCoins"); if(!p&&PyErr_Occurred()) return nullptr;
    p->deployCoins(first,second,third,fourth); return none();
}
static PyObject* PowerUps_generateCoinTable(PyObject*,PyObject* args) {
    char* encoded; float radius; int teeIndex=1; char* surface=nullptr;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sf|is:PowerUps_generateCoinTable"),&encoded,&radius,&teeIndex,&surface)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_generateCoinTable"); if(!p&&PyErr_Occurred()) return nullptr;
    p->generateCoinTable(radius,teeIndex,surface); return none();
}
static PyObject* PowerUps_collide(PyObject*,PyObject* args) {
    char *encoded,*firstText,*secondText; float radius;
    if(!PyArg_ParseTuple(args,const_cast<char*>("sssf:PowerUps_collide"),&encoded,&firstText,&secondText,&radius)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_collide"); if(!p&&PyErr_Occurred()) return nullptr;
    if(!checkPointer(firstText,"_Point3F_p","PowerUps_collide",2)||!checkPointer(secondText,"_Point3F_p","PowerUps_collide",3)) return nullptr;
    auto* first=static_cast<Point3F*>(decodeReceiver(firstText,"_Point3F_p","PowerUps_collide"));
    auto* second=static_cast<Point3F*>(decodeReceiver(secondText,"_Point3F_p","PowerUps_collide"));
    bool result=false;
    if(first&&second) result=first->x+radius>=second->x && first->x-radius<=second->x &&
        first->y+radius>=second->y && first->y-radius<=second->y &&
        first->z+radius>=second->z && first->z-radius<=second->z;
    return Py_BuildValue(const_cast<char*>("i"),result?1:0);
}

static PyObject* PowerUps_enable(PyObject*,PyObject* args) {
    char* encoded; int value;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:PowerUps_enable"),&encoded,&value)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_enable"); if (!p&&PyErr_Occurred()) return nullptr;
    p->powersEnabled=value!=0; return none();
}
static PyObject* PowerUps_isEnabled(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_isEnabled"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_isEnabled"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->powersEnabled?1:0);
}
static PyObject* PowerUps_enableCoins(PyObject*,PyObject* args) {
    char* encoded; int value;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:PowerUps_enableCoins"),&encoded,&value)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_enableCoins"); if (!p&&PyErr_Occurred()) return nullptr;
    p->coinsEnabled=value!=0; return none();
}
static PyObject* PowerUps_isSlowMotion(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_isSlowMotion"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_isSlowMotion"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->slowMotion?1:0);
}
static PyObject* PowerUps_setSlowMotion(PyObject*,PyObject* args) {
    char* encoded; int value;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:PowerUps_setSlowMotion"),&encoded,&value)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_setSlowMotion"); if (!p&&PyErr_Occurred()) return nullptr;
    p->slowMotion=value!=0; return none();
}
static PyObject* PowerUps_setDefaultPickUpCommand(PyObject*,PyObject* args) {
    char *encoded,*taskText;
    if (!PyArg_ParseTuple(args,const_cast<char*>("ss:PowerUps_setDefaultPickUpCommand"),&encoded,&taskText)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_setDefaultPickUpCommand"); if (!p&&PyErr_Occurred()) return nullptr;
    if (!checkPointer(taskText,"_Task_p","PowerUps_setDefaultPickUpCommand",2)) return nullptr;
    auto* task=static_cast<Task*>(decodeReceiver(taskText,"_Task_p","PowerUps_setDefaultPickUpCommand"));
    p->defaultPickupTask=task; return none();
}
static PyObject* PowerUps_setPickUpCoinCommand(PyObject*,PyObject* args) {
    char *encoded,*taskText;
    if (!PyArg_ParseTuple(args,const_cast<char*>("ss:PowerUps_setPickUpCoinCommand"),&encoded,&taskText)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_setPickUpCoinCommand"); if (!p&&PyErr_Occurred()) return nullptr;
    if (!checkPointer(taskText,"_Task_p","PowerUps_setPickUpCoinCommand",2)) return nullptr;
    auto* task=static_cast<Task*>(decodeReceiver(taskText,"_Task_p","PowerUps_setPickUpCoinCommand"));
    p->coinPickupTask=task; return none();
}
static PyObject* PowerUps_createPower(PyObject*,PyObject* args) {
    char *encoded,*name,*taskText; int kind;
    if (!PyArg_ParseTuple(args,const_cast<char*>("siss:PowerUps_createPower"),&encoded,&kind,&name,&taskText)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_createPower"); if (!p&&PyErr_Occurred()) return nullptr;
    if (!checkPointer(taskText,"_Task_p","PowerUps_createPower",4)) return nullptr;
    auto* task=static_cast<Task*>(decodeReceiver(taskText,"_Task_p","PowerUps_createPower"));
    return Py_BuildValue(const_cast<char*>("i"),p->createPower(static_cast<unsigned>(kind),name,task));
}
static PyObject* PowerUps_reset(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_reset"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_reset"); if (!p&&PyErr_Occurred()) return nullptr;
    p->reset(); return none();
}
static PyObject* PowerUps_createCoin(PyObject*,PyObject* args) {
    char *encoded,*name; int id,kind;
    if (!PyArg_ParseTuple(args,const_cast<char*>("siis:PowerUps_createCoin"),&encoded,&id,&kind,&name)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_createCoin"); if (!p&&PyErr_Occurred()) return nullptr;
    p->createCoin(id,static_cast<unsigned>(kind),name); return none();
}
static PyObject* PowerUps_getPowersCount(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_getPowersCount"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_getPowersCount"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->powerDefinitionCount);
}
static PyObject* PowerUps_getPowerUpCount(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_getPowerUpCount"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_getPowerUpCount"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->powerInstanceCount);
}
static PyObject* PowerUps_getCurrentPowerUp(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_getCurrentPowerUp"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_getCurrentPowerUp"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->getCurrentPowerUp());
}
static PyObject* PowerUps_getCurrentCoin(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_getCurrentCoin"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_getCurrentCoin"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->getCurrentCoin());
}
static PyObject* PowerUps_removeCurrentPowerUp(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_removeCurrentPowerUp"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_removeCurrentPowerUp"); if (!p&&PyErr_Occurred()) return nullptr;
    p->removeCurrentPowerUp(); return none();
}
static PyObject* PowerUps_removeCurrentCoin(PyObject*,PyObject* args) {
    char* encoded; if (!PyArg_ParseTuple(args,const_cast<char*>("s:PowerUps_removeCurrentCoin"),&encoded)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_removeCurrentCoin"); if (!p&&PyErr_Occurred()) return nullptr;
    p->removeCurrentCoin(); return none();
}
static PyObject* PowerUps_getName(PyObject*,PyObject* args) {
    char* encoded; int id;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:PowerUps_getName"),&encoded,&id)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_getName"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("s"),p->getName(id));
}
static PyObject* PowerUps_getCoinName(PyObject*,PyObject* args) {
    char* encoded; int id;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:PowerUps_getCoinName"),&encoded,&id)) return nullptr;
    auto* p=receiver<PowerUps>(encoded,"_PowerUps_p","PowerUps_getCoinName"); if (!p&&PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("s"),p->getCoinName(id));
}
}
PyMethodDef* remainingGameMethods() {
    static PyMethodDef methods[]={
#define ENTRY(T,N) {const_cast<char*>(#T "_" #N),T##_##N,METH_VARARGS,nullptr},
        ENTRY(Scene,root_set) ENTRY(Scene,root_get) ENTRY(Scene,load) ENTRY(Scene,createTransform)
        ENTRY(Scene,createAnim) ENTRY(Scene,hideFakeObjects) ENTRY(Scene,hide) ENTRY(Scene,show) ENTRY(Scene,setCamera)
        ENTRY(guiElement,setWriteOnlyAttr) ENTRY(guiElement,specialCmd) ENTRY(guiBindings,add) ENTRY(guiBindings,remove)
        ENTRY(guiSystem,setBackground) ENTRY(guiSystem,addElement) ENTRY(guiSystem,removeElement) ENTRY(guiSystem,setCurrentTab)
        ENTRY(guiSystem,findNearestTab) ENTRY(guiSystem,findMouseOver) ENTRY(guiSystem,shellCmd) ENTRY(guiSystem,dtsSetSkin)
        ENTRY(guiSystem,dtsLoad) ENTRY(guiSystem,dtsCmd) ENTRY(guiSystem,dtsFlush) ENTRY(guiSystem,getStringWidth)
        ENTRY(guiSystem,getStringHeight) ENTRY(guiSystem,preloadZoom) ENTRY(guiSystem,playZoom)
        ENTRY(Hole,init) ENTRY(Hole,getName) ENTRY(Hole,getPar) ENTRY(Hole,setAmbientColour) ENTRY(AI,cmd)
        ENTRY(PowerUps,enable) ENTRY(PowerUps,isEnabled) ENTRY(PowerUps,enableCoins) ENTRY(PowerUps,isSlowMotion)
        ENTRY(PowerUps,setSlowMotion) ENTRY(PowerUps,setDefaultPickUpCommand) ENTRY(PowerUps,setPickUpCoinCommand)
        ENTRY(PowerUps,createPower) ENTRY(PowerUps,reset) ENTRY(PowerUps,createCoin) ENTRY(PowerUps,addHoleCoin)
        ENTRY(PowerUps,getPowersCount) ENTRY(PowerUps,getPowerUpCount) ENTRY(PowerUps,getCurrentPowerUp)
        ENTRY(PowerUps,getCurrentCoin) ENTRY(PowerUps,removeCurrentPowerUp) ENTRY(PowerUps,removeCurrentCoin)
        ENTRY(PowerUps,getName) ENTRY(PowerUps,getCoinName) ENTRY(PowerUps,deployCoins) ENTRY(PowerUps,generateCoinTable)
        ENTRY(PowerUps,collide)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
