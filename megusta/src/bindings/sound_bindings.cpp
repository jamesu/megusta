#include "sound_bindings.h"
#include "runtime.h"
#include "game/sounds.h"
#include "game/scene_triangle.h"
#include <cstdio>
namespace megusta::bindings {
namespace { Sounds* service=nullptr; }
void bindSoundServices(Sounds* value) { service=value; }
namespace {
#include "wrapper_macros.h"
#define NEED_SOUND if (!service) { PyErr_SetString(PyExc_RuntimeError,"Game sound services are not bound"); return nullptr; }
Point3F* pointArgument(const char* text,const char* function) {
    auto* point=receiver<Point3F>(text,"_Point3F_p",function);
    if (!point && PyErr_Occurred()) {
        char error[160]; std::sprintf(error,"Type error in argument 3 of %s. Expected _Point3F_p.",function);
        PyErr_SetString(PyExc_TypeError,error);
    }
    return point;
}
SWIG_BEGIN(Sounds,play,"ssi",char* name; int loop,&name,&loop) NEED_SOUND service->play(name,loop); return none(); }
SWIG_BEGIN(Sounds,playStream,"ssi",char* name; int loop,&name,&loop) NEED_SOUND service->playStream(name,loop); return none(); }
SWIG_BEGIN(Sounds,playVoice,"ssi",char* name; int delay,&name,&delay) NEED_SOUND service->playVoice(name,delay); return none(); }
SWIG_BEGIN(Sounds,playAnnouncer,"ss",char* name,&name) NEED_SOUND service->playAnnouncer(name); return none(); }
SWIG_BEGIN(Sounds,stop,"ss|i",char* name; int key=0,&name,&key) NEED_SOUND service->stop(name,key); return none(); }
SWIG_BEGIN(Sounds,isPlaying,"ss|i",char* name; int key=0,&name,&key) NEED_SOUND return Py_BuildValue("i",int(service->isPlaying(name,key))); }
SWIG_BEGIN(Sounds,preload,"ss|i",char* name; int group=2,&name,&group) NEED_SOUND return Py_BuildValue("i",int(service->preload(name,group))); }
SWIG_BEGIN(Sounds,preloadVoice,"ss|i",char* name; int group=2,&name,&group) NEED_SOUND return Py_BuildValue("i",int(service->preloadVoice(name,group))); }
SETINT(Sounds,unloadGroup,NEED_SOUND service->unloadGroup(value))
SWIG_BEGIN(Sounds,play3d,"sss|ii",char* name; char* location; int loop=0; int key=0,&name,&location,&loop,&key)
    auto* point=pointArgument(location,"Sounds_play3d"); if (!point && PyErr_Occurred()) return nullptr;
    NEED_SOUND service->play3d(name,point->x,point->y,point->z,loop,key); return none(); }
SWIG_BEGIN(Sounds,setPos,"sss|i",char* name; char* location; int key=0,&name,&location,&key)
    auto* point=pointArgument(location,"Sounds_setPos"); if (!point && PyErr_Occurred()) return nullptr;
    NEED_SOUND service->setPos(name,point->x,point->y,point->z,key); return none(); }
}
PyMethodDef* soundBindingMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>("Sounds_" #N),Sounds_##N,METH_VARARGS,nullptr},
        ENTRY(play) ENTRY(playStream) ENTRY(playVoice) ENTRY(playAnnouncer) ENTRY(stop) ENTRY(isPlaying)
        ENTRY(preload) ENTRY(preloadVoice) ENTRY(unloadGroup) ENTRY(play3d) ENTRY(setPos)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
