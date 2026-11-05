#include "bindings/world.h"
#include "game/scene_object.h"
#include "game/player.h"
#include "game/camera.h"
#include "game/putter.h"
#include "game/driver.h"
#include "ssgGameAnimation.h"
#include <cstdio>
#include <cstring>
int checkWorldBindings() {
    int count=0;
    PyMethodDef* tables[]={megusta::bindings::ssgBindingMethods(),megusta::bindings::objectPlayerMethods(),megusta::bindings::cameraPutterDriverMethods()};
    for (auto* table:tables) for (auto* method=table;method->ml_name;++method) {
        PyObject* args=!std::strcmp(method->ml_name,"puttmap_dbase_isCreating")
            ?Py_BuildValue(const_cast<char*>("(i)"),1):PyTuple_New(0);
        PyObject* result=method->ml_meth(nullptr,args); Py_DECREF(args);
        if (result || !PyErr_Occurred()) { Py_XDECREF(result); return 30; }
        PyErr_Clear(); ++count;
    }
    ssgBranch root;
    auto* transform=new ssgTransform; transform->ref();
    // setUserData retains/releases the animator through the PLIB node.
    auto* animation=new ssgAnim(transform,3);
    megusta::SceneObjectRecord object[3]{};
    object[1].name="object-one"; object[1].entity=&root; object[1].frameCount=7;
    object[2].name="object-two"; object[2].entity=transform;
    sgMakeIdentMat4(object[1].frameTransform); sgMakeIdentMat4(object[1].baseFrameTransform);
    megusta::SceneObjectCollection objects{}; objects.objects=object; objects.objectCount=2;
    megusta::PlayersManager players{}; players.playerCount=2; players.turnState=4; players.displayedStrokeCount=9;
    players.playerStorage=new megusta::Player[4]{}; players.currentPlayer=players.playerStorage;
    megusta::Camera camera{}; camera.controlEye={1,2,3}; camera.controlLookAt={4,5,6};
    megusta::Putter putter; putter.teeObject=&object[1]; putter.power=0.75f; putter.state=2;
    megusta::Driver driver{}; std::strcpy(driver.activeScript,"test-module");
    char setup[1536];
    std::sprintf(setup,
        "root = '_%lx_ssgEntity_p'\ntr = '_%lx_ssgTransform_p'\nan = '_%lx_ssgAnim_p'\nobj = '_%lx_Object_p'\nfriend = '_%lx_Object_p'\nobjects = '_%lx_Objects_p'\nplayers = '_%lx_Players_p'\ncam = '_%lx_Camera_p'\nputter = '_%lx_Putter_p'\ndriver = '_%lx_Driver_p'\n",
        reinterpret_cast<unsigned long>(&root),reinterpret_cast<unsigned long>(transform),
        reinterpret_cast<unsigned long>(animation),reinterpret_cast<unsigned long>(&object[1]),
        reinterpret_cast<unsigned long>(&object[2]),reinterpret_cast<unsigned long>(&objects),
        reinterpret_cast<unsigned long>(&players),reinterpret_cast<unsigned long>(&camera),
        reinterpret_cast<unsigned long>(&putter),reinterpret_cast<unsigned long>(&driver));
    if (PyRun_SimpleString(setup)) { PyErr_Print(); return 31; }
    const char* checks=
        "assert gamec.puttmap_dbase_isCreating() == 0\n"
        "gamec.ssgEntity_setName(tr, 'child')\n"
        "gamec.ssgEntity_addKid(root, tr)\nassert gamec.ssgEntity_getNumKids(root) == 1\n"
        "assert gamec.ssgEntity_getKid(root, -1) == 'NULL'\n"
        "assert gamec.ssgEntity_getByName(root, 'child') == gamec.ssgEntity_getKid(root, 0)\n"
        "gamec.ssgTransform_fastSet(tr, 1, 2, 3, 90)\npos = gamec.ssgTransform_getPos(tr)\n"
        "assert gamec.Point3F_x_get(pos) == 1\nassert gamec.Point3F_y_get(pos) == 2\n"
        "point = gamec.new_Point3F(4, 5, 6)\ngamec.ssgTransform_setPos(tr, point)\n"
        "assert gamec.Point3F_x_get(gamec.ssgTransform_getPos(tr)) == 4\n"
        "assert gamec.ssgAnim_getNumFrames(an) == 3\ngamec.ssgAnim_setFrame(an, 2)\n"
        "gamec.ssgAnim_nextFrame(an)\nassert gamec.ssgAnim_getFrame(an) == 0\n"
        "assert gamec.Object_getName(obj) == 'object-one'\n"
        "gamec.Object_hide(obj)\nassert gamec.Object_isHidden(obj) == 1\n"
        "gamec.Object_show(obj)\nassert gamec.Object_isHidden(obj) == 0\n"
        "gamec.Object_setSync(obj, 1)\ngamec.Object_setSeeThru(obj)\n"
        "gamec.Object_setCOR(obj, 0.25)\ngamec.Object_setCOF(obj, 0.5)\n"
        "assert gamec.Object_getNumFrames(obj) == 7\nassert gamec.Object_getFrame(obj) == 0\n"
        "gamec.Object_setTrigger(obj, 'NULL')\n"
        "gamec.Object_addSeeThruFriend(obj, friend)\ngamec.Object_addSeeThruFriend(obj, friend)\n"
        "assert gamec.Objects_getCount(objects) == 2\nassert gamec.Objects_get(objects, 1) == obj\n"
        "assert gamec.Players_getCount(players) == 2\nassert gamec.Players_getCurrent(players) != 'NULL'\n"
        "assert gamec.Players_getDisplayStroke(players) == 9\nassert gamec.Players_isTurnEnded(players) == 1\n"
        "gamec.Players_setGlobalHandedness(players, 1)\ngamec.Players_setEventHandler(players, 0, 'NULL')\n"
        "gamec.Camera_setHeading(cam, 37)\nassert gamec.Camera_getHeading(cam) == 37\n"
        "gamec.Camera_setEyeDist(cam, -3)\n"
        "eye = gamec.Camera_getLoc(cam)\ntarget = gamec.Camera_getTarget(cam)\n"
        "assert gamec.Point3F_z_get(eye) == 3\nassert gamec.Point3F_y_get(target) == 5\n"
        "gamec.Point3F_x_set(eye, 99)\n"
        "assert gamec.Putter_getTee(putter) == obj\nassert gamec.Putter_getPower(putter) == 0.75\n"
        "assert gamec.Putter_isAiming(putter) == 0\nassert gamec.Putter_isSwinging(putter) == 1\n"
        "assert gamec.Putter_isPutting(putter) == 1\ngamec.Putter_showPowerBar(putter, 1)\n"
        "assert gamec.Driver_getScriptName(driver) == 'test-module'\ngamec.Driver_showFPS(driver, 3)\n"
        "gamec.ssgEntity_removeKid(root, tr)\nassert gamec.ssgEntity_getNumKids(root) == 0\n"
        "gamec.ssgEntity_removeAllKids(root)\n"
        "gamec.delete_Point3F(point)\ngamec.delete_Point3F(eye)\ngamec.delete_Point3F(target)\n";
    const int scriptResult=PyRun_SimpleString(const_cast<char*>(checks));
    if (scriptResult) PyErr_Print();
    bool fields=object[1].coefficientOfRestitution==0.25f && object[1].coefficientOfFriction==0.5f &&
        (object[1].flags&0xc0u)==0xc0u && object[1].seeThruFriendCount==1 && object[2].seeThruFriendCount==1 &&
        camera.controlEye.x==1 && camera.savedHeadingDegrees==37 && camera.followDistance==-3 &&
        putter.powerBarVisible && driver.fpsOverlayEnabled && !driver.frameLimitEnabled && players.holeFlag;
    players.reset();
    ssgDeRefDelete(transform);
    if (scriptResult) return 32;
    if (!fields) return 33;
    std::printf("world bindings: %d argument guards, SSG dispatch, animation wrapping, object fields, camera ownership and control flags passed\n",count);
    return count==100?0:34;
}
