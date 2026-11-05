#include "game_basic.h"
#include "runtime.h"
#include "config_tasks.h"
#include "gameplay.h"
#include "world.h"
#include "sound_bindings.h"
#include "game_variables.h"
#include "swig_runtime.h"
#include "gui_bindings.h"
#include "remaining_game_bindings.h"
#include "game/effect.h"
#include "game/console.h"
#include <new>
namespace megusta::bindings {
namespace {
struct GameConstant { const char* name; long value; };

// Exact integer namespace installed by swig_init_gamec_bindings at 00429900.
// These are Python module attributes, separate from the dynamic `cvar` object.
constexpr GameConstant gameConstants[]={
    {"guiElement_INTERFACE",0},{"guiElement_FRAME",1},{"guiElement_TEXT",2},
    {"guiElement_BUTTON",3},{"guiElement_TOGGLE",4},{"guiElement_INPUT",5},
    {"guiElement_SLIDER",6},{"guiElement_SELECT",7},{"guiElement_OPTION",8},
    {"guiElement_FILEPICKER",9},{"guiElement_HEADER",10},{"guiElement_SHADOWBOX",11},
    {"guiElement_STATBAR",12},{"guiElement_BTN_LOZENGE",13},
    {"guiElement_BTN_LOZENGE2",14},{"guiElement_BTN_SIMPLE",15},
    {"guiElement_BTN_BMPANIM",16},{"guiElement_BTN_BMP",17},
    {"guiElement_BTN_3DSCENE",18},{"guiElement_BTN_LIST",19},
    {"guiElement_BTN_BALLGAD",20},{"guiElement_NUM_TYPES",21},
    {"guiElement_A_NAME",0},{"guiElement_A_LEGEND",1},{"guiElement_A_LABEL",2},
    {"guiElement_A_VALUE",3},{"guiElement_A_IMAGE",4},{"guiElement_A_HINT",5},
    {"guiElement_NUM_ATTR",6},{"guiElement_A_WO_ABSPOS",0},
    {"guiElement_A_WO_INCPOS",1},{"guiElement_A_WO_ACTIVE",2},
    {"guiElement_A_WO_VISIBLE",3},{"guiElement_A_WO_FLAGS",4},
    {"guiElement_A_WO_SORT",5},{"guiElement_ON_CHANGE",0},
    {"guiElement_ON_CLICK",1},{"guiElement_ON_SUBMIT",2},{"guiElement_NUM_EH",3},
    {"Putter_SEQ_NONE",-1},{"Putter_SEQ_PUTT",0},{"Putter_SEQ_PAR",1},
    {"Putter_SEQ_BOGEY",2},{"Putter_SEQ_BORED",3},{"Putter_SEQ_PRE",4},
    {"Putter_SEQ_BIRD",6},{"Putter_SEQ_MISS",7},{"Putter_SEQ_PAR2",8},
    {"Putter_SEQ_SHL",9},{"Putter_NUM_SEQS",10},
    {"Camera_NOAUTOCAM",0},{"Camera_AUTOCAM",1},{"Camera_SKYCAM",2},
    {"Camera_BALLCAM",3},{"Camera_SECURECAM",4},{"Camera_PLAYERCAM",5},
    {"Camera_FROZENCAM",6},{"Camera_CINEMACAM",7},{"Camera_CONTROLCAM",8},
    {"Camera_OUTCAM",9},{"Camera_ZOOMCAM",10},{"Camera_REPLAYCAM",11},
    {"Camera_THEMATRIXMOVIECAM",12},{"Camera_NCAM_STATES",18},
    {"Players_ON_HIT_PLAYER",0},{"Players_ON_HIT_OOBOUNDS",1},
    {"Players_ON_NEXT_PLAYER",2},{"Players_ON_NEXT_HOLE",3},
    {"Players_NUM_EH",4}
};

bool installGameConstants(PyObject* module) {
    PyObject* dictionary=PyModule_GetDict(module);
    for (const auto& constant:gameConstants) {
        PyObject* value=PyInt_FromLong(constant.value);
        if (!value) return false;
        const int result=PyDict_SetItemString(dictionary,
            const_cast<char*>(constant.name),value);
        Py_DECREF(value);
        if (result<0) return false;
    }
    return true;
}

struct TypeMappingInit { const char* source; const char* equivalent; SwigCast cast; };
void registerGamecTypeMappings() {
    // The pairs below are the address-backed registrations from
    // swig_init_gamec_bindings at 00429900, normalized to their surviving
    // type-string values. Equivalent aliases remain grouped by source type.
    static const TypeMappingInit mappings[]={
        {"_signed_long","_long",nullptr},{"_double","_f64",nullptr},
        {"_class_Effect","_Effect",nullptr},{"_class_Scene","_Scene",nullptr},
        {"_class_Rnd","_Rnd",nullptr},{"_class_Object","_Object",nullptr},
        {"_guiSystem","_class_guiSystem",nullptr},
        {"_class_fntFont","_class_fntTexFont",swig_identity_upcast},
        {"_class_fntFont","_fntTexFont",swig_identity_upcast},
        {"_class_fntFont","_fntFont",nullptr},
        {"_class_fntTexFont","_fntTexFont",nullptr},
        {"_long","_unsigned_long",nullptr},{"_long","_signed_long",nullptr},
        {"_Sounds","_class_Sounds",nullptr},{"_Player","_class_Player",nullptr},
        {"_guiElement","_class_guiElement",nullptr},{"_float","_f32",nullptr},
        {"_GameConfig","_class_GameConfig",nullptr},{"_Players","_class_Players",nullptr},
        {"_AI","_class_AI",nullptr},{"_Ball","_class_Ball",nullptr},
        {"_signed_char","_s8",nullptr},{"_fntTexFont","_class_fntTexFont",nullptr},
        {"_class_ssgAnim","_ssgAnim",nullptr},{"_class_guiSystem","_guiSystem",nullptr},
        {"_class_Task","_Task",nullptr},{"_class_Sounds","_Sounds",nullptr},
        {"_class_Balls","_Balls",nullptr},{"_class_Player","_Player",nullptr},
        {"_ssgRoot","_class_ssgRoot",nullptr},
        {"_class_guiBindings","_guiBindings",nullptr},{"_char","_cchar",nullptr},
        {"_Putter","_class_Putter",nullptr},{"_fntRenderer","_class_fntRenderer",nullptr},
        {"_Driver","_class_Driver",nullptr},{"_class_Tasks","_Tasks",nullptr},
        {"_class_PowerUps","_PowerUps",nullptr},{"_s16","_signed_short",nullptr},
        {"_s16","_short",nullptr},{"_Hole","_class_Hole",nullptr},
        {"_class_guiElement","_guiElement",nullptr},{"_unsigned_long","_long",nullptr},
        {"_Console","_class_Console",nullptr},{"_class_GameConfig","_GameConfig",nullptr},
        {"_class_Players","_Players",nullptr},{"_class_Putter","_Putter",nullptr},
        {"_class_Ball","_Ball",nullptr},{"_signed_int","_s32",nullptr},
        {"_signed_int","_int",nullptr},{"_Scene","_class_Scene",nullptr},
        {"_s32","_signed_int",nullptr},{"_s32","_int",nullptr},
        {"_class_Driver","_Driver",nullptr},{"_unsigned_short","_u16",nullptr},
        {"_unsigned_short","_short",nullptr},{"_Point3F","_class_Point3F",nullptr},
        {"_ssgEntity","_class_ssgRoot",swig_identity_upcast},
        {"_ssgEntity","_ssgRoot",swig_identity_upcast},
        {"_ssgEntity","_class_ssgTransform",swig_identity_upcast},
        {"_ssgEntity","_ssgTransform",swig_identity_upcast},
        {"_ssgEntity","_class_ssgEntity",nullptr},
        {"_class_ssgRoot","_ssgRoot",nullptr},
        {"_ssgTransform","_class_ssgTransform",nullptr},
        {"_signed_short","_s16",nullptr},{"_signed_short","_short",nullptr},
        {"_unsigned_char","_u8",nullptr},{"_unsigned_int","_u32",nullptr},
        {"_unsigned_int","_int",nullptr},{"_Camera","_class_Camera",nullptr},
        {"_Objects","_class_Objects",nullptr},{"_short","_u16",nullptr},
        {"_short","_s16",nullptr},{"_short","_unsigned_short",nullptr},
        {"_short","_signed_short",nullptr},{"_f32","_float",nullptr},
        {"_class_Hole","_Hole",nullptr},{"_Balls","_class_Balls",nullptr},
        {"_class_ssgEntity","_class_ssgRoot",swig_identity_upcast},
        {"_class_ssgEntity","_ssgRoot",swig_identity_upcast},
        {"_class_ssgEntity","_class_ssgTransform",swig_identity_upcast},
        {"_class_ssgEntity","_ssgTransform",swig_identity_upcast},
        {"_class_ssgEntity","_ssgEntity",nullptr},
        {"_guiBindings","_class_guiBindings",nullptr},
        {"_int","_u32",nullptr},{"_int","_s32",nullptr},
        {"_int","_unsigned_int",nullptr},{"_int","_signed_int",nullptr},
        {"_class_ssgTransform","_ssgTransform",nullptr},
        {"_class_Console","_Console",nullptr},{"_class_AI","_AI",nullptr},
        {"_u16","_unsigned_short",nullptr},{"_u16","_short",nullptr},
        {"_class_Camera","_Camera",nullptr},{"_Tasks","_class_Tasks",nullptr},
        {"_s8","_signed_char",nullptr},{"_cchar","_char",nullptr},
        {"_fntFont","_class_fntTexFont",swig_identity_upcast},
        {"_fntFont","_fntTexFont",swig_identity_upcast},
        {"_fntFont","_class_fntFont",nullptr},
        {"_PowerUps","_class_PowerUps",nullptr},
        {"_class_Point3F","_Point3F",nullptr},
        {"_u32","_unsigned_int",nullptr},{"_u32","_int",nullptr},
        {"_Effect","_class_Effect",nullptr},{"_f64","_double",nullptr},
        {"_Rnd","_class_Rnd",nullptr},{"_class_fntRenderer","_fntRenderer",nullptr},
        {"_class_Objects","_Objects",nullptr},{"_ssgAnim","_class_ssgAnim",nullptr},
        {"_Object","_class_Object",nullptr},{"_u8","_unsigned_char",nullptr},
        {"_Task","_class_Task",nullptr}
    };
    for (const auto& mapping:mappings)
        swig_register_type_mapping(mapping.source,mapping.equivalent,mapping.cast);
}

#define CHECK(T,N) auto* p=receiver<T>(encoded,"_" #T "_p",#T "_" #N); if (!p && PyErr_Occurred()) return nullptr;
#define EMPTY(N) static PyObject* Effect_##N(PyObject*,PyObject* args) { char* encoded; \
    if (!PyArg_ParseTuple(args,"s:Effect_" #N,&encoded)) return nullptr; \
    CHECK(Effect,N) p->N(); return none(); }
#define SCALAR(N,E) static PyObject* Effect_##N(PyObject*,PyObject* args) { char* encoded; float value; \
    if (!PyArg_ParseTuple(args,"sf:Effect_" #N,&encoded,&value)) return nullptr; \
    CHECK(Effect,N) E; return none(); }
#define TRIPLE(N) static PyObject* Effect_##N(PyObject*,PyObject* args) { char* encoded; float x,y,z; \
    if (!PyArg_ParseTuple(args,"sfff:Effect_" #N,&encoded,&x,&y,&z)) return nullptr; \
    CHECK(Effect,N) p->N(x,y,z); return none(); }
#include "effect_methods.inc"
#undef EMPTY
#undef SCALAR
#undef TRIPLE
static PyObject* new_Effect(PyObject*,PyObject* args) {
    if (!PyArg_ParseTuple(args,":new_Effect")) return nullptr;
    return pointerResult(new(std::nothrow) Effect,"_Effect_p");
}
static PyObject* Effect_warmup(PyObject*,PyObject* args) {
    char* encoded; int count=0;
    if (!PyArg_ParseTuple(args,"s|i:Effect_warmup",&encoded,&count)) return nullptr;
    CHECK(Effect,warmup) p->warmup(count); return none();
}
static PyObject* Effect_setTexture(PyObject*,PyObject* args) {
    char *encoded,*name;
    if (!PyArg_ParseTuple(args,"ss:Effect_setTexture",&encoded,&name)) return nullptr;
    CHECK(Effect,setTexture) p->setTexture(name); return none();
}
static PyObject* Effect_setPosition(PyObject*,PyObject* args) {
    char* encoded; float x,y,z,angle=0;
    if (!PyArg_ParseTuple(args,"sfff|f:Effect_setPosition",&encoded,&x,&y,&z,&angle)) return nullptr;
    CHECK(Effect,setPosition) p->setPosition(x,y,z,angle); return none();
}
static PyObject* Console_echo(PyObject*,PyObject* args) {
    char *encoded,*text;
    if (!PyArg_ParseTuple(args,"ss:Console_echo",&encoded,&text)) return nullptr;
    CHECK(Console,echo) p->echo(text); return none();
}
#undef CHECK
}
PyMethodDef* basicGameMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(new_Effect) ENTRY(Effect_warmup) ENTRY(Effect_setTexture)
        ENTRY(Effect_setPosition) ENTRY(Console_echo)
#define EMPTY(N) ENTRY(Effect_##N)
#define SCALAR(N,E) ENTRY(Effect_##N)
#define TRIPLE(N) ENTRY(Effect_##N)
#include "effect_methods.inc"
#undef EMPTY
#undef SCALAR
#undef TRIPLE
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
PyObject* initializeBasicGameModule() {
    swig_init_gamec_bindings(false);
    return PyErr_Occurred()?nullptr:PyImport_AddModule(const_cast<char*>("gamec"));
}
void swig_init_gamec_bindings(bool reload) {
    resetGameVariables();
    if (reload) return;
    PyObject* module=Py_InitModule4(const_cast<char*>("gamec"),basicGameMethods(),nullptr,nullptr,1007);
    if (!module) return;
    if (!installGameConstants(module)) return;
    // Py_InitModule4 adds methods to the existing module rather than replacing
    // its dictionary, matching CPython's extension registration mechanism.
    if (!Py_InitModule4(const_cast<char*>("gamec"),configTaskMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),pointFontMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),playerBallMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),ssgBindingMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),objectPlayerMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),cameraPutterDriverMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),soundBindingMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),guiBindingMethods(),nullptr,nullptr,1007)) return;
    if (!Py_InitModule4(const_cast<char*>("gamec"),remainingGameMethods(),nullptr,nullptr,1007)) return;
    if (!installGameVariables(module)) return;
    registerGamecTypeMappings();
}
}
