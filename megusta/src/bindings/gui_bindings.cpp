#include "gui_bindings.h"
#include "runtime.h"
#include "game/gui.h"
#include "game/input.h"
#include "ssg.h"
namespace megusta::bindings {
namespace {
#include "wrapper_macros.h"
INTEGER(guiElement,getWidth,p->getWidth())
INTEGER(guiElement,getHeight,p->getHeight())
SWIG_BEGIN(guiElement,getAttr,"si",int which,&which) return Py_BuildValue("s",p->getAttr(which)); }
SWIG_BEGIN(guiElement,setAttr,"sis",int which; char* value,&which,&value) p->setAttr(which,value); return none(); }
SWIG_BEGIN(guiElement,getEH,"si",int which,&which) return Py_BuildValue("s",static_cast<char*>(nullptr)); } // 0040cb40 returns zero.
SWIG_BEGIN(guiElement,setEH,"sis",int which; char* value,&which,&value) p->setEH(which,value); return none(); }
SWIG_BEGIN(guiElement,setPosition,"sii",int x; int y,&x,&y) p->setPosition(x,y); return none(); }
SIMPLE(guiElement,hide,p->hide(); return none())
SIMPLE(guiElement,show,p->show(); return none())
SIMPLE(guiBindings,clear,p->clear(); return none())
SIMPLE(guiBindings,push,p->push(); return none())
SIMPLE(guiBindings,pop,p->pop(); return none())
POINTER(guiSystem,bindings_get,&p->bindings,guiBindings)
SWIG_BEGIN(guiSystem,bindings_set,"ss",char* encodedBindings,&encodedBindings)
    auto* value=receiver<guiBindings>(encodedBindings,"_guiBindings_p","guiSystem_bindings_set");
    if (!value && PyErr_Occurred()) return nullptr;
    p->bindings=*value; // Original copies the complete 64-byte record shallowly.
    return pointerResult(value,"_guiBindings_p"); }
SWIG_BEGIN(guiSystem,findElement,"ss",char* name,&name) return pointerResult(p->findElement(name),"_guiElement_p"); }
TEXTUAL(guiSystem,getCurrent,p->currentElement?p->currentElement->attributes[0]:nullptr)
INTEGER(guiSystem,getKey,gameInput.keyCode)
INTEGER(guiSystem,altPressed,gameInput.modifiers&1)
SIMPLE(guiSystem,getRoot,if (!p->previewRoot) p->previewRoot=new ssgRoot; return pointerResult(p->previewRoot,"_ssgRoot_p"))
}
PyMethodDef* guiBindingMethods() {
    static PyMethodDef methods[]={
#define ENTRY(T,N) {const_cast<char*>(#T "_" #N),T##_##N,METH_VARARGS,nullptr},
        ENTRY(guiElement,getWidth) ENTRY(guiElement,getHeight) ENTRY(guiElement,getAttr) ENTRY(guiElement,setAttr)
        ENTRY(guiElement,getEH) ENTRY(guiElement,setEH) ENTRY(guiElement,setPosition) ENTRY(guiElement,hide) ENTRY(guiElement,show)
        ENTRY(guiBindings,clear) ENTRY(guiBindings,push) ENTRY(guiBindings,pop)
        ENTRY(guiSystem,bindings_get) ENTRY(guiSystem,bindings_set) ENTRY(guiSystem,findElement) ENTRY(guiSystem,getCurrent)
        ENTRY(guiSystem,getKey) ENTRY(guiSystem,altPressed) ENTRY(guiSystem,getRoot)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
