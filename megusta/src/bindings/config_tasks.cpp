#include "config_tasks.h"
#include "runtime.h"
#include "game/game_config.h"
#include "game/task.h"
namespace megusta::bindings {
namespace {
ResourceMountTable* resources=nullptr;
TaskRuntime* tasks=nullptr;
PyObject* missingService() {
    PyErr_SetString(PyExc_RuntimeError,"Native config/task services are not bound"); return nullptr;
}
// Native arrays contain borrowed strings in static scratch storage. Python
// copies each string, and PyList_SetItem steals the newly allocated reference.
PyObject* stringList(char** values) {
    int count=0; while (values[count]) ++count;
    PyObject* list=PyList_New(count); if (!list) return nullptr;
    for (int i=0;i<count;++i) {
        PyObject* item=PyString_FromString(values[i]);
        if (!item) { Py_DECREF(list); return nullptr; }
        PyList_SetItem(list,i,item);
    }
    return list;
}
#define RECEIVER(T,N) auto* p=receiver<T>(encoded,"_" #T "_p",#T "_" #N); if (!p && PyErr_Occurred()) return nullptr;
#define SIMPLE(T,N,E) static PyObject* T##_##N(PyObject*,PyObject* args) { char* encoded; \
    if (!PyArg_ParseTuple(args,const_cast<char*>("s:" #T "_" #N),&encoded)) return nullptr; \
    RECEIVER(T,N) E; }
SIMPLE(GameConfig,clear,p->clear(); return none())
SIMPLE(GameConfig,push,p->push(); return none())
SIMPLE(GameConfig,pop,p->pop(); return none())
SIMPLE(GameConfig,load,if (!resources) return missingService(); return Py_BuildValue(const_cast<char*>("i"),int(p->load(*resources))))
SIMPLE(GameConfig,save,if (!resources) return missingService(); return Py_BuildValue(const_cast<char*>("i"),int(p->save(*resources))))
SIMPLE(GameConfig,readSections,return stringList(p->readSections()))
static PyObject* GameConfig_readKeys(PyObject*,PyObject* args) {
    char *encoded,*section;
    if (!PyArg_ParseTuple(args,const_cast<char*>("ss:GameConfig_readKeys"),&encoded,&section)) return nullptr;
    RECEIVER(GameConfig,readKeys) return stringList(p->readKeys(section));
}
static PyObject* GameConfig_readString(PyObject*,PyObject* args) {
    char *encoded,*section,*key,*fallback=nullptr;
    if (!PyArg_ParseTuple(args,const_cast<char*>("sss|s:GameConfig_readString"),&encoded,&section,&key,&fallback)) return nullptr;
    RECEIVER(GameConfig,readString) return Py_BuildValue(const_cast<char*>("s"),p->readString(section,key,fallback));
}
static PyObject* GameConfig_writeString(PyObject*,PyObject* args) {
    char *encoded,*section,*key,*value;
    if (!PyArg_ParseTuple(args,const_cast<char*>("ssss:GameConfig_writeString"),&encoded,&section,&key,&value)) return nullptr;
    RECEIVER(GameConfig,writeString) p->writeString(section,key,value); return none();
}
static PyObject* GameConfig_readInt(PyObject*,PyObject* args) {
    char *encoded,*section,*key; int fallback=0;
    if (!PyArg_ParseTuple(args,const_cast<char*>("sss|i:GameConfig_readInt"),&encoded,&section,&key,&fallback)) return nullptr;
    RECEIVER(GameConfig,readInt) return Py_BuildValue(const_cast<char*>("i"),p->readInt(section,key,fallback));
}
static PyObject* GameConfig_writeInt(PyObject*,PyObject* args) {
    char *encoded,*section,*key; int value;
    if (!PyArg_ParseTuple(args,const_cast<char*>("sssi:GameConfig_writeInt"),&encoded,&section,&key,&value)) return nullptr;
    RECEIVER(GameConfig,writeInt) p->writeInt(section,key,value); return none();
}
SIMPLE(Task,getBall,return pointerResult(p->contextObject,"_Ball_p"))
// contextTag is the original pointer-sized object token, retained as uint32 on
// the supported Win32 target. Its alternate last-touch usage remains tracked.
SIMPLE(Task,getObject,return pointerResult(reinterpret_cast<void*>(static_cast<std::uintptr_t>(p->contextTag)),"_Object_p"))
SIMPLE(Task,isPaused,return Py_BuildValue(const_cast<char*>("i"),int((p->stateFlags&2u)==2u)))
SIMPLE(Task,pause,p->stateFlags|=2u; return none())
SIMPLE(Task,unPause,p->stateFlags&=~2u; return none())
SIMPLE(Task,isRunning,return Py_BuildValue(const_cast<char*>("i"),int((p->stateFlags&1u)==1u)))
SIMPLE(Task,start,if (!tasks) return missingService(); p->start(*tasks); return none())
SIMPLE(Task,stop,p->stop(); return none())
static PyObject* Task_setMask(PyObject*,PyObject* args) {
    char* encoded; int value;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:Task_setMask"),&encoded,&value)) return nullptr;
    RECEIVER(Task,setMask) p->setMask(static_cast<std::uint32_t>(value)); return none();
}
#undef SIMPLE
static PyObject* Tasks_getCount(PyObject*,PyObject* args) {
    char* encoded;
    if (!PyArg_ParseTuple(args,const_cast<char*>("s:Tasks_getCount"),&encoded)) return nullptr;
    auto* p=receiver<TasksManager>(encoded,"_Tasks_p","Tasks_getCount");
    if (!p && PyErr_Occurred()) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),p->taskCount);
}
static PyObject* Tasks_getCurrent(PyObject*,PyObject* args) {
    char* encoded;
    if (!PyArg_ParseTuple(args,const_cast<char*>("s:Tasks_getCurrent"),&encoded)) return nullptr;
    auto* p=receiver<TasksManager>(encoded,"_Tasks_p","Tasks_getCurrent");
    if (!p && PyErr_Occurred()) return nullptr;
    return pointerResult(p->currentTask,"_Task_p");
}
// Original three bridge bodies append first, then retain the callable.
Task* appendPythonScheduled(TasksManager& manager,PyObject* callable,int mode) {
    Task* task=manager.appendScheduled(invokePythonTaskCallback,reinterpret_cast<std::uintptr_t>(callable),mode);
    Py_INCREF(callable);return task;
}
Task* appendPythonIdle(TasksManager& manager,PyObject* callable) {
    Task* task=manager.appendIdle(invokePythonTaskCallback,reinterpret_cast<std::uintptr_t>(callable),*tasks);
    Py_INCREF(callable);return task;
}
Task* appendPythonDraw(TasksManager& manager,PyObject* callable) {
    Task* task=manager.appendDraw(invokePythonTaskCallback,reinterpret_cast<std::uintptr_t>(callable),*tasks);
    Py_INCREF(callable);return task;
}
#define ADD(N,F,DECL,ARGS,E) static PyObject* Tasks_##N(PyObject*,PyObject* args) { \
    char* encoded; PyObject* callable; DECL; \
    if (!PyArg_ParseTuple(args,const_cast<char*>(F ":Tasks_" #N),&encoded,&callable ARGS)) return nullptr; \
    auto* p=receiver<TasksManager>(encoded,"_Tasks_p","Tasks_" #N); \
    if (!p && PyErr_Occurred()) return nullptr; \
    if (!PyCallable_Check(callable)) { PyErr_SetString(PyExc_TypeError,"Need a callable object!"); return nullptr; } \
    if (!tasks) return missingService(); return pointerResult(E,"_Task_p"); }
#define MODE_ARG ,&mode
ADD(add,"sOi",int mode,MODE_ARG,appendPythonScheduled(*p,callable,mode))
ADD(addIdle,"sO",,,appendPythonIdle(*p,callable))
ADD(addDraw2d,"sO",,,appendPythonDraw(*p,callable))
#undef MODE_ARG
#undef ADD
#undef RECEIVER
}
void bindConfigTaskServices(ResourceMountTable* mounts,TaskRuntime* runtime) { resources=mounts; tasks=runtime; }
PyMethodDef* configTaskMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(GameConfig_clear) ENTRY(GameConfig_load) ENTRY(GameConfig_save)
        ENTRY(GameConfig_push) ENTRY(GameConfig_pop) ENTRY(GameConfig_readSections)
        ENTRY(GameConfig_readKeys) ENTRY(GameConfig_readString) ENTRY(GameConfig_writeString)
        ENTRY(GameConfig_readInt) ENTRY(GameConfig_writeInt)
        ENTRY(Task_getBall) ENTRY(Task_getObject) ENTRY(Task_isPaused)
        ENTRY(Task_pause) ENTRY(Task_unPause) ENTRY(Task_isRunning)
        ENTRY(Task_start) ENTRY(Task_stop) ENTRY(Task_setMask)
        ENTRY(Tasks_getCount) ENTRY(Tasks_getCurrent) ENTRY(Tasks_add)
        ENTRY(Tasks_addIdle) ENTRY(Tasks_addDraw2d)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
