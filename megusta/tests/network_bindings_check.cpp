#include "bindings/network.h"
#include "bindings/game_basic.h"
#include "bindings/config_tasks.h"
#include "game/room_client.h"
#include "game/effect.h"
#include "game/task.h"
#include "engine/resource_runtime.h"
#include "runtime_setup.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
int checkGameplayBindings();
int checkWorldBindings();
int checkNetworkBindings() {
    megusta::ResourceRuntime io;
    io.initializeFromArgs(0,nullptr);
    const char* classExceptions=std::getenv("MEGUSTA_TEST_CLASS_EXCEPTIONS");
    megusta::configureGamePython(classExceptions && !std::strcmp(classExceptions,"1"));
    // Testing-only override: these checks intentionally use Python asserts.
    Py_OptimizeFlag=0;
    Py_Initialize();
    if (!megusta::bindings::initializeNetworkModule()) { PyErr_Print(); return 1; }
    int count=0;
    // Every wrapper must reject an empty tuple before touching native state.
    for (auto* method=megusta::bindings::networkMethods();method->ml_name;++method) {
        PyObject* args=PyTuple_New(0);
        PyObject* result=method->ml_meth(nullptr,args); Py_DECREF(args);
        if (result || !PyErr_Occurred()) { Py_XDECREF(result); return 2; }
        PyErr_Clear(); ++count;
    }
    megusta::iRoom room;
    std::strcpy(room.name,"binding-room"); room.num_games=7;
    char setup[512];
    std::sprintf(setup,"import netc\nr = '_%llx_iRoom_p'\na = '_%llx_class_iRoom_p'\n",
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(&room)),
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(&room)));
    if (PyRun_SimpleString(setup)) { PyErr_Print(); return 3; }
    const char* checks=
        "assert netc.iRoom_getName(r) == 'binding-room'\n"
        "assert netc.iRoom_getNumGames(a) == 7\n"
        "assert netc.cvar.link == 'NULL'\n"
        "try:\n netc.cvar.link = 'NULL'\nexcept TypeError:\n pass\nelse:\n raise RuntimeError, 'link accepted a write'\n"
        "try:\n netc.iRoom_getName('_1_iGame_p')\nexcept TypeError:\n pass\nelse:\n raise RuntimeError, 'bad tag accepted'\n"
        "try:\n netc.iRoomClient_serviceStart('NULL', ())\nexcept TypeError:\n pass\nelse:\n raise RuntimeError, 'non-list accepted'\n"
        "try:\n netc.iRoomClient_serviceStart('NULL', [1])\nexcept TypeError:\n pass\nelse:\n raise RuntimeError, 'non-string accepted'\n"
        "assert netc.iRoomClient_serviceStart('NULL', ['test']) == 0\n"
        "assert netc.iRoomClient_serviceEnd('NULL') is None\n";
    if (PyRun_SimpleString(const_cast<char*>(checks))) { PyErr_Print(); return 4; }
    std::printf("network bindings: %d argument guards, pointer aliases, fields, cvar and service arguments passed\n",count);
    if (!megusta::bindings::initializeBasicGameModule()) { PyErr_Print(); return 5; }
    int gameCount=0;
    for (auto* method=megusta::bindings::basicGameMethods();method->ml_name;++method) {
        // new_Effect accepts no arguments; give it a deliberately wrong tuple.
        PyObject* args=!std::strcmp(method->ml_name,"new_Effect")
            ?Py_BuildValue(const_cast<char*>("(i)"),1):PyTuple_New(0);
        PyObject* result=method->ml_meth(nullptr,args); Py_DECREF(args);
        if (result || !PyErr_Occurred()) { Py_XDECREF(result); return 6; }
        PyErr_Clear(); ++gameCount;
    }
    megusta::Effect effect(2.0f); // Unregistered native constructor needs no global runtime.
    std::sprintf(setup,"import gamec\ne = '_%llx_Effect_p'\n",
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(&effect)));
    if (PyRun_SimpleString(setup)) { PyErr_Print(); return 7; }
    const char* effectChecks=
        "assert gamec.Effect_setPosition(e, 1, 2, 3) is None\n"
        "gamec.Effect_setVolume(e, 4, 5, 6)\n"
        "gamec.Effect_setGravity(e, 7, 8, 9)\n"
        "gamec.Effect_setScale(e, 2)\n"
        "gamec.Effect_setDuration(e, 17.9)\n"
        "gamec.Effect_setMomentum(e, 3)\n"
        "gamec.Effect_warmup(e)\n"
        "gamec.Effect_stop(e)\n";
    if (PyRun_SimpleString(const_cast<char*>(effectChecks))) { PyErr_Print(); return 8; }
    if (effect.position[0]!=1 || effect.position[2]!=3 || effect.angleDegrees!=0 ||
        effect.emissionVolume[1]!=5 || effect.gravity[2]!=9 || effect.scale[0]!=2 ||
        effect.durationTicks!=17 || effect.emissionState!=-1) return 9;
    std::printf("basic game bindings: %d argument guards, optional arguments and native Effect field writes passed\n",gameCount);
    int configTaskCount=0;
    for (auto* method=megusta::bindings::configTaskMethods();method->ml_name;++method) {
        PyObject* args=PyTuple_New(0);
        PyObject* result=method->ml_meth(nullptr,args); Py_DECREF(args);
        if (result || !PyErr_Occurred()) { Py_XDECREF(result); return 10; }
        PyErr_Clear(); ++configTaskCount;
    }
    megusta::GameConfig config(nullptr);
    megusta::TasksManager manager{};
    megusta::TaskRuntime runtime{nullptr,manager,
        [](void*)->megusta::Ball* { return nullptr; },
        [](void*,megusta::Ball*)->std::uintptr_t { return 0; },
        [](void*){},megusta::invokePythonTaskCallback,megusta::releasePythonTaskCallback};
    manager.clear(runtime);
    megusta::bindings::bindConfigTaskServices(nullptr,&runtime);
    std::sprintf(setup,"c = '_%llx_GameConfig_p'\nts = '_%llx_Tasks_p'\n",
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(&config)),
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(&manager)));
    if (PyRun_SimpleString(setup)) { PyErr_Print(); return 11; }
    const char* configChecks=
        "assert gamec.GameConfig_readSections(c) == []\n"
        "assert gamec.GameConfig_readInt(c, 'S', 'missing') == 0\n"
        "assert gamec.GameConfig_readString(c, 'S', 'text', 'fallback') == 'fallback'\n"
        "assert gamec.GameConfig_readString(c, 'S', 'null') == ''\n"
        "gamec.GameConfig_writeString(c, 'S', 'text', 'original')\n"
        "gamec.GameConfig_push(c)\n"
        "gamec.GameConfig_writeString(c, 'S', 'text', 'temporary')\n"
        "assert gamec.GameConfig_readString(c, 'S', 'text') == 'temporary'\n"
        "gamec.GameConfig_pop(c)\n"
        "assert gamec.GameConfig_readString(c, 'S', 'text') == 'original'\n"
        "gamec.GameConfig_writeInt(c, 'S', 'number', -17)\n"
        "assert gamec.GameConfig_readInt(c, 'S', 'number') == -17\n"
        "assert 'number' in gamec.GameConfig_readKeys(c, 'S')\n"
        "assert gamec.GameConfig_readSections(c) == ['S']\n"
        "try:\n gamec.GameConfig_load(c)\nexcept RuntimeError:\n pass\nelse:\n raise RuntimeError, 'unbound IO accepted'\n"
        "calls = []\ndef callback():\n calls.append('called')\n";
    if (PyRun_SimpleString(const_cast<char*>(configChecks))) { PyErr_Print(); return 12; }
    PyObject* mainDict=PyModule_GetDict(PyImport_AddModule(const_cast<char*>("__main__")));
    PyObject* callback=PyDict_GetItemString(mainDict,const_cast<char*>("callback"));
    const int references=callback->ob_refcnt;
    const char* taskChecks=
        "try:\n gamec.Tasks_add(ts, 7, 0)\nexcept TypeError:\n pass\nelse:\n raise RuntimeError, 'non-callable accepted'\n"
        "t = gamec.Tasks_add(ts, callback, 0)\n"
        "assert gamec.Tasks_getCount(ts) == 1\n"
        "assert gamec.Tasks_getCurrent(ts) == 'NULL'\n"
        "assert gamec.Task_isRunning(t) == 0\n"
        "gamec.Task_pause(t)\nassert gamec.Task_isPaused(t) == 1\n"
        "gamec.Task_unPause(t)\nassert gamec.Task_isPaused(t) == 0\n"
        "gamec.Task_start(t)\nassert calls == ['called']\n"
        "assert gamec.Task_isRunning(t) == 0\n"
        "assert gamec.Task_getBall(t) == 'NULL'\n"
        "assert gamec.Task_getObject(t) == 'NULL'\n"
        "gamec.Task_setMask(t, -1)\n"
        "idle = gamec.Tasks_addIdle(ts, callback)\n"
        "draw = gamec.Tasks_addDraw2d(ts, callback)\n"
        "gamec.Task_stop(idle)\nassert gamec.Task_isRunning(idle) == 0\n";
    if (PyRun_SimpleString(const_cast<char*>(taskChecks))) { PyErr_Print(); return 13; }
    if (callback->ob_refcnt!=references+3 || manager.slots[1].mask!=0xffffffffu) return 14;
    manager.runDrawCallbacks(runtime);
    if (PyRun_SimpleString(const_cast<char*>("assert calls == ['called', 'called']\n"))) { PyErr_Print(); return 15; }
    manager.clear(runtime);
    if (callback->ob_refcnt!=references) return 16;
    megusta::bindings::bindConfigTaskServices(nullptr,nullptr);
    std::printf("config/task bindings: %d argument guards, config lists/defaults, callbacks, flags and retained references passed\n",configTaskCount);
    const int gameplayResult=checkGameplayBindings();
    const int worldResult=gameplayResult?0:checkWorldBindings();
    Py_Finalize(); return gameplayResult?gameplayResult:worldResult;
}
