#include "game_variables.h"
#include "runtime.h"
#include "swig_runtime.h"
#include "game/driver.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace megusta::bindings {
const AICommandPort aiCommandPort{};
namespace {
struct Entry { const char* name; const char* type; int slot; };
const Entry entries[]={
    {"scene","_Scene_p",0},{"rnd","_Rnd_p",1},{"fluffRnd","_Rnd_p",2},
    {"driver","_Driver_p",3},{"config","_GameConfig_p",4},{"gui","_guiSystem_p",5},
    {"console","_Console_p",6},{"sounds","_Sounds_p",7},{"tasks","_Tasks_p",8},
    {"font1","_fntTexFont_p",9},{"font2","_fntTexFont_p",10},{"font3","_fntTexFont_p",11},
    {"fontRenderer","_fntRenderer_p",12},{"camera","_Camera_p",13},
    {"objects","_Objects_p",14},{"balls","_Balls_p",15},{"players","_Players_p",16},
    {"putter","_Putter_p",17},{"hole","_Hole_p",18},{"ai","_AI_p",19},
    {"powerups","_PowerUps_p",20}
};
GameVariables* selected=nullptr;
SwigVarLink* gamecLink=nullptr;

PyObject* missingBinding(const char* name) {
    char message[128];
    std::sprintf(message,"Game global variable %s is not bound.",name);
    PyErr_SetString(PyExc_RuntimeError,message);
    return nullptr;
}
PyObject* getObject(int slot,const char* name,const char* type) {
    if (!selected || !selected->bound[slot]) return missingBinding(name);
    char encoded[128];
    swig_encode_pointer(encoded,selected->objects[slot],type);
    return PyString_FromString(encoded);
}
int readOnly(const char* name) {
    char message[128];
    std::sprintf(message,"Variable %s is read-only.",name);
    PyErr_SetString(PyExc_TypeError,message);
    return 1;
}
#define OBJECT_CVAR(Name,Slot,Type) \
    PyObject* get_##Name() { return getObject(Slot,#Name,Type); } \
    int set_##Name(PyObject*) { return readOnly(#Name); }
OBJECT_CVAR(scene,0,"_Scene_p")
OBJECT_CVAR(rnd,1,"_Rnd_p")
OBJECT_CVAR(fluffRnd,2,"_Rnd_p")
OBJECT_CVAR(driver,3,"_Driver_p")
OBJECT_CVAR(config,4,"_GameConfig_p")
OBJECT_CVAR(gui,5,"_guiSystem_p")
OBJECT_CVAR(console,6,"_Console_p")
OBJECT_CVAR(sounds,7,"_Sounds_p")
OBJECT_CVAR(tasks,8,"_Tasks_p")
OBJECT_CVAR(font1,9,"_fntTexFont_p")
OBJECT_CVAR(font2,10,"_fntTexFont_p")
OBJECT_CVAR(font3,11,"_fntTexFont_p")
OBJECT_CVAR(fontRenderer,12,"_fntRenderer_p")
OBJECT_CVAR(camera,13,"_Camera_p")
OBJECT_CVAR(objects,14,"_Objects_p")
OBJECT_CVAR(balls,15,"_Balls_p")
OBJECT_CVAR(players,16,"_Players_p")
OBJECT_CVAR(putter,17,"_Putter_p")
OBJECT_CVAR(hole,18,"_Hole_p")
OBJECT_CVAR(ai,19,"_AI_p")
OBJECT_CVAR(powerups,20,"_PowerUps_p")
#undef OBJECT_CVAR
int viewportValue(int index) {
    if (!selected || !selected->viewport) {
        PyErr_SetString(PyExc_RuntimeError,"Game viewport is not bound.");
        return 0;
    }
    const DriverViewport& view=*selected->viewport;
    switch (index) {
    case 0: return view.displayWidth;
    case 1: return view.displayHeight;
    case 2: return view.viewportWidth;
    default: return view.viewportHeight;
    }
}
#define DIMENSION_CVAR(Name,Index) \
    PyObject* get_##Name() { const int value=viewportValue(Index); \
        return PyErr_Occurred()?nullptr:PyInt_FromLong(value); } \
    int set_##Name(PyObject*) { return readOnly(#Name); }
DIMENSION_CVAR(SCRN_WIDTH,0)
DIMENSION_CVAR(SCRN_HEIGHT,1)
DIMENSION_CVAR(VIEWPORT_WIDTH,2)
DIMENSION_CVAR(VIEWPORT_HEIGHT,3)
#undef DIMENSION_CVAR
}

bool GameVariables::set(const char* name,const void* value) {
    for (const auto& entry:entries) if (std::strcmp(name,entry.name)==0) {
        objects[entry.slot]=value;
        bound[entry.slot]=true;
        return true;
    }
    return false;
}
void bindGameVariables(GameVariables* value) { selected=value; }

void resetGameVariables() {
    if (gamecLink) swig_varlink_destroy(gamecLink);
    gamecLink=nullptr;
}
bool installGameVariables(PyObject* module) {
    resetGameVariables();
    gamecLink=swig_varlink_new();
    if (!gamecLink) return false;
    swig_track_varlink(&gamecLink,gamecLink);
#define ADD(Name) swig_varlink_add(gamecLink,#Name,get_##Name,set_##Name)
    ADD(scene); ADD(rnd); ADD(fluffRnd); ADD(driver); ADD(config); ADD(gui);
    ADD(console); ADD(sounds); ADD(tasks); ADD(font1); ADD(font2); ADD(font3);
    ADD(fontRenderer); ADD(camera); ADD(objects); ADD(balls); ADD(players);
    ADD(putter); ADD(hole); ADD(ai); ADD(powerups);
    ADD(SCRN_WIDTH); ADD(SCRN_HEIGHT); ADD(VIEWPORT_WIDTH); ADD(VIEWPORT_HEIGHT);
#undef ADD
    const int result=PyDict_SetItemString(PyModule_GetDict(module),const_cast<char*>("cvar"),
        reinterpret_cast<PyObject*>(gamecLink));
    if (result<0) { resetGameVariables(); return false; }
    return true;
}
}
