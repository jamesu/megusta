#include "network.h"
#include "runtime.h"
#include "swig_runtime.h"
#include "game/room_client.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
namespace megusta { extern RoomClientRuntime* roomClientRuntime; }
namespace megusta::bindings {
void* decodeReceiver(const char* encoded,const char* tag,const char* function) {
    void* value=nullptr;
    char* mismatch=swig_decode_pointer(encoded,&value,tag);
    if (!mismatch) return value;
    char message[256];
    std::sprintf(message,"Type error in argument 1 of %s. Expected %s.",function,tag);
    PyErr_SetString(PyExc_TypeError,message); return nullptr;
}
PyObject* pointerResult(const void* pointer,const char* tag) {
    char encoded[128];
    swig_encode_pointer(encoded,pointer,tag);
    return Py_BuildValue("s",encoded);
}
PyObject* none() { Py_INCREF(Py_None); return Py_None; }
namespace {
PyObject* netc_cvar_link_get() {
    char encoded[128];
    swig_encode_pointer(encoded,roomClientRuntime?&roomClientRuntime->client:nullptr,"_iRoomClient_p");
    return PyString_FromString(encoded);
}
int netc_cvar_link_set(PyObject*) {
    PyErr_SetString(PyExc_TypeError,"Variable link is read-only.");
    return 1;
}
SwigVarLink* networkVariableLink=nullptr;
// NULL is a valid decoded pointer; distinguish it from a type error without
// introducing a new receiver validation absent from the original wrappers.
#define BEGIN(T,N,F,...) static PyObject* T##_##N(PyObject*,PyObject* args) { \
    char* encoded; if (!PyArg_ParseTuple(args,F ":" #T "_" #N,&encoded, ##__VA_ARGS__)) return nullptr; \
    auto* p=receiver<T>(encoded,"_" #T "_p",#T "_" #N); if (!p && PyErr_Occurred()) return nullptr;
#define INT(T,N,E) BEGIN(T,N,"s") return Py_BuildValue("i",static_cast<int>(E)); }
#define TEXT(T,N,E) BEGIN(T,N,"s") return Py_BuildValue("s",E); }
#define VOID(T,N,E) BEGIN(T,N,"s") E; return none(); }
#define POINTER(T,N,E,R) BEGIN(T,N,"s") return pointerResult(E,"_" #R "_p"); }
#define FIND(N,R) static PyObject* iRoomClient_##N(PyObject*,PyObject* args) { char *encoded,*value; \
    if (!PyArg_ParseTuple(args,"ss:iRoomClient_" #N,&encoded,&value)) return nullptr; \
    auto* p=receiver<iRoomClient>(encoded,"_iRoomClient_p","iRoomClient_" #N); \
    if (!p && PyErr_Occurred()) return nullptr; return pointerResult(p->N(value),"_" #R "_p"); }
#define STRING_VOID(N,E) static PyObject* iRoomClient_##N(PyObject*,PyObject* args) { char *encoded,*value; \
    if (!PyArg_ParseTuple(args,"ss:iRoomClient_" #N,&encoded,&value)) return nullptr; \
    auto* p=receiver<iRoomClient>(encoded,"_iRoomClient_p","iRoomClient_" #N); \
    if (!p && PyErr_Occurred()) return nullptr; E; return none(); }
#include "network_methods.inc"
#undef INT
#undef TEXT
#undef VOID
#undef POINTER
#undef FIND
#undef STRING_VOID
#undef BEGIN
// Multi-argument wrappers retain Python 1.5.2 parsing and borrowed storage.
#define CLIENT(N,F,DECL,...) static PyObject* iRoomClient_##N(PyObject*,PyObject* args) { \
    char* encoded; DECL; if (!PyArg_ParseTuple(args,F ":iRoomClient_" #N,&encoded,__VA_ARGS__)) return nullptr; \
    auto* p=receiver<iRoomClient>(encoded,"_iRoomClient_p","iRoomClient_" #N); if (!p && PyErr_Occurred()) return nullptr;
CLIENT(registerNick,"sss",char* nick; char* password,&nick,&password)
    p->setNick(nick); return none(); }
CLIENT(identifyNick,"sss",char* nick; char* password,&nick,&password)
    p->setNick(nick); return none(); }
CLIENT(enterRoom,"ssss",char* player; char* room; char* password,&player,&room,&password)
    p->enterRoom(player,room,password); return none(); }
CLIENT(createGame,"sssi",char* name; char* password; int maximum,&name,&password,&maximum)
    p->createGame(name,password,maximum); return none(); }
CLIENT(joinGame,"sss",char* name; char* password,&name,&password)
    p->joinGame(name,password); return none(); }
CLIENT(watchGame,"sss",char* name; char* password,&name,&password)
    p->watchGame(name,password); return none(); }
CLIENT(sendInvite,"sss",char* name; char* password,&name,&password)
    p->sendInvite(name,password); return none(); }
CLIENT(sayTo,"sss",char* text; char* recipient,&text,&recipient)
    p->sayTo(text,recipient); return none(); }
CLIENT(getSaid,"si",int index,&index)
    return Py_BuildValue("s",p->getSaid(index)); }
CLIENT(getIgnore,"ss",char* name,&name)
    return Py_BuildValue("i",static_cast<int>(p->getIgnore(name))); }
CLIENT(setIgnore,"ssi",char* name; int value,&name,&value)
    p->setIgnore(name,value); return none(); }
CLIENT(setChatUp,"si",int value,&value)
    p->chatUp=value; return none(); }
CLIENT(isDead,"si",int index,&index)
    return Py_BuildValue("i",static_cast<int>(p->isDead(index))); }
CLIENT(serviceStart,"sO",PyObject* list,&list)
    if (list->ob_type!=&PyList_Type) { PyErr_SetString(PyExc_TypeError,"not a list"); return nullptr; }
    const int count=PyList_Size(list);
    auto** argv=static_cast<char**>(std::malloc((count+1)*sizeof(char*)));
    for (int i=0;i<count;++i) {
        PyObject* item=PyList_GetItem(list,i);
        if (item->ob_type!=&PyString_Type) {
            PyErr_SetString(PyExc_TypeError,"list must contain strings"); std::free(argv); return nullptr;
        }
        argv[i]=PyString_AsString(item);
    }
    argv[count]=nullptr;
    // The original service callback is a no-op and ignores argc/argv.
    const int result=networkServiceStart(); std::free(argv); return Py_BuildValue("i",result); }
#undef CLIENT
static PyObject* iRoomClient_serviceEnd(PyObject*,PyObject* args) {
    char* encoded;
    if (!PyArg_ParseTuple(args,"s:iRoomClient_serviceEnd",&encoded)) return nullptr;
    auto* p=receiver<iRoomClient>(encoded,"_iRoomClient_p","iRoomClient_serviceEnd");
    if (!p && PyErr_Occurred()) return nullptr;
    networkServiceEnd(); return none();
}
}
PyMethodDef* networkMethods() {
    static PyMethodDef methods[]={
#define EXTRA(N) {const_cast<char*>("iRoomClient_" #N),iRoomClient_##N,METH_VARARGS,nullptr},
        EXTRA(registerNick) EXTRA(identifyNick) EXTRA(enterRoom) EXTRA(createGame)
        EXTRA(joinGame) EXTRA(watchGame) EXTRA(sendInvite) EXTRA(sayTo)
        EXTRA(getSaid) EXTRA(getIgnore) EXTRA(setIgnore) EXTRA(setChatUp)
        EXTRA(isDead) EXTRA(serviceStart) EXTRA(serviceEnd)
#undef EXTRA
#define ROW(T,N) {const_cast<char*>(#T "_" #N),T##_##N,METH_VARARGS,nullptr},
#define INT(T,N,E) ROW(T,N)
#define TEXT(T,N,E) ROW(T,N)
#define VOID(T,N,E) ROW(T,N)
#define POINTER(T,N,E,R) ROW(T,N)
#define FIND(N,R) ROW(iRoomClient,N)
#define STRING_VOID(N,E) ROW(iRoomClient,N)
#include "network_methods.inc"
#undef ROW
#undef INT
#undef TEXT
#undef VOID
#undef POINTER
#undef FIND
#undef STRING_VOID
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
PyObject* initializeNetworkModule() {
    swig_init_network_bindings(false);
    return PyErr_Occurred()?nullptr:PyImport_AddModule(const_cast<char*>("netc"));
}
void swig_init_network_bindings(bool reload) {
    if (networkVariableLink) swig_varlink_destroy(networkVariableLink);
    networkVariableLink=nullptr;
    if (reload) return;
    PyObject* module=Py_InitModule4(const_cast<char*>("netc"),networkMethods(),nullptr,nullptr,1007);
    if (!module) return;
    networkVariableLink=swig_varlink_new();
    if (!networkVariableLink) return;
    swig_track_varlink(&networkVariableLink,networkVariableLink);
    swig_varlink_add(networkVariableLink,"link",netc_cvar_link_get,netc_cvar_link_set);
    if (PyDict_SetItemString(PyModule_GetDict(module),const_cast<char*>("cvar"),
            reinterpret_cast<PyObject*>(networkVariableLink))<0) {
        swig_varlink_destroy(networkVariableLink);
        networkVariableLink=nullptr;
        return;
    }
    // Network SWIG scalar and interface spellings retained by its initializer.
    swig_register_type_mapping("_signed_long","_long",nullptr);
    swig_register_type_mapping("_double","_f64",nullptr);
    swig_register_type_mapping("_long","_unsigned_long",nullptr);
    swig_register_type_mapping("_long","_signed_long",nullptr);
    swig_register_type_mapping("_iGame","_class_iGame",nullptr);
    swig_register_type_mapping("_float","_f32",nullptr);
    swig_register_type_mapping("_signed_char","_s8",nullptr);
    swig_register_type_mapping("_iRoomClient","_class_iRoomClient",nullptr);
    swig_register_type_mapping("_iRoom","_class_iRoom",nullptr);
    swig_register_type_mapping("_char","_cchar",nullptr);
    swig_register_type_mapping("_int","_signed_int",nullptr);
    swig_register_type_mapping("_int","_short",nullptr);
    swig_register_type_mapping("_unsigned_long","_long",nullptr);
    swig_register_type_mapping("_signed_int","_s32",nullptr);
    swig_register_type_mapping("_signed_int","_int",nullptr);
    swig_register_type_mapping("_s32","_signed_int",nullptr);
    swig_register_type_mapping("_s32","_int",nullptr);
    swig_register_type_mapping("_unsigned_short","_u16",nullptr);
    swig_register_type_mapping("_unsigned_short","_short",nullptr);
    swig_register_type_mapping("_signed_short","_s16",nullptr);
    swig_register_type_mapping("_signed_short","_short",nullptr);
    swig_register_type_mapping("_s16","_signed_short",nullptr);
    swig_register_type_mapping("_s16","_short",nullptr);
    swig_register_type_mapping("_iPlayer","_class_iPlayer",nullptr);
    swig_register_type_mapping("_unsigned_char","_u8",nullptr);
    swig_register_type_mapping("_s8","_signed_char",nullptr);
    swig_register_type_mapping("_cchar","_char",nullptr);
    swig_register_type_mapping("_unsigned_int","_u32",nullptr);
    swig_register_type_mapping("_unsigned_int","_int",nullptr);
    swig_register_type_mapping("_short","_u16",nullptr);
    swig_register_type_mapping("_short","_int",nullptr);
    swig_register_type_mapping("_short","_unsigned_short",nullptr);
    swig_register_type_mapping("_short","_signed_short",nullptr);
    swig_register_type_mapping("_f32","_float",nullptr);
    swig_register_type_mapping("_class_iGame","_iGame",nullptr);
    swig_register_type_mapping("_int","_u32",nullptr);
    swig_register_type_mapping("_int","_s32",nullptr);
    swig_register_type_mapping("_u16","_unsigned_short",nullptr);
    swig_register_type_mapping("_u16","_short",nullptr);
    swig_register_type_mapping("_class_iRoomClient","_iRoomClient",nullptr);
    swig_register_type_mapping("_class_iRoom","_iRoom",nullptr);
    swig_register_type_mapping("_u32","_unsigned_int",nullptr);
    swig_register_type_mapping("_u32","_int",nullptr);
    swig_register_type_mapping("_f64","_double",nullptr);
    swig_register_type_mapping("_class_iPlayer","_iPlayer",nullptr);
    swig_register_type_mapping("_u8","_unsigned_char",nullptr);
}
}
