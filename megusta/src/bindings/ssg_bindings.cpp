#include "world.h"
#include "runtime.h"
#include "game/scene_triangle.h"
#include "ssgGameAnimation.h"
#include <cstdio>
namespace megusta::bindings {
namespace {
#include "wrapper_macros.h"
template<class T> T* argument(const char* value,const char* tag,const char* name,int number) {
    auto* p=receiver<T>(value,tag,name);
    if (!p && PyErr_Occurred()) { char message[256];
        std::sprintf(message,"Type error in argument %d of %s. Expected %s.",number,name,tag);
        PyErr_SetString(PyExc_TypeError,message);
    }
    return p;
}
ssgBranch* branch(ssgEntity* p) { return p->isAKindOf(ssgTypeBranch())?static_cast<ssgBranch*>(p):nullptr; }
SWIG_BEGIN(ssgEntity,getByName,"ss",char* name,&name) return pointerResult(p->getByName(name),"_ssgEntity_p"); }
SWIG_BEGIN(ssgEntity,setName,"ss",char* name,&name) p->setName(name); return none(); }
INTEGER(ssgEntity,getNumKids,branch(p)?branch(p)->getNumKids():0)
SWIG_BEGIN(ssgEntity,getKid,"si",int index,&index)
    return pointerResult(branch(p)?branch(p)->getKid(index):nullptr,"_ssgEntity_p"); }
#define CHILD(N) SWIG_BEGIN(ssgEntity,N,"ss",char* value,&value) \
    auto* child=argument<ssgEntity>(value,"_ssgEntity_p","ssgEntity_" #N,2); \
    if (!child && PyErr_Occurred()) return nullptr; if (branch(p)) branch(p)->N(child); return none(); }
CHILD(addKid) CHILD(removeKid)
#undef CHILD
SIMPLE(ssgEntity,removeAllKids,if (branch(p)) branch(p)->removeAllKids(); return none())
SWIG_BEGIN(ssgTransform,set,"sffff",float x; float y; float z; float angle,&x,&y,&z,&angle)
    sgVec3 axis={0,0,1}; sgMat4 rotation,translation;
    sgMakeRotMat4(rotation,angle,axis); sgMakeTransMat4(translation,x,y,z);
    sgPostMultMat4(rotation,translation); p->setTransform(rotation); return none(); }
SWIG_BEGIN(ssgTransform,fastSet,"sffff",float x; float y; float z; float ignoredAngle,&x,&y,&z,&ignoredAngle)
    sgMat4 matrix; sgMakeIdentMat4(matrix); matrix[3][0]=x; matrix[3][1]=y; matrix[3][2]=z;
    p->setTransform(matrix); return none(); }
SIMPLE(ssgTransform,getPos,static Point3F scratch;
    sgMat4 matrix; p->getTransform(matrix); scratch.x=matrix[3][0]; scratch.y=matrix[3][1]; scratch.z=matrix[3][2];
    return pointerResult(&scratch,"_Point3F_p"))
SWIG_BEGIN(ssgTransform,setPos,"ss",char* value,&value)
    auto* point=argument<Point3F>(value,"_Point3F_p","ssgTransform_setPos",2);
    if (!point && PyErr_Occurred()) return nullptr;
    sgMat4 matrix; p->getTransform(matrix); matrix[3][0]=point->x; matrix[3][1]=point->y; matrix[3][2]=point->z;
    p->setTransform(matrix); return none(); }
INTEGER(ssgAnim,getNumFrames,p->getNumFrames())
INTEGER(ssgAnim,getFrame,p->getFrame())
SETINT(ssgAnim,setFrame,p->setFrame(static_cast<unsigned>(value)))
SIMPLE(ssgAnim,nextFrame,unsigned next=p->getFrame()+1; if (next>=p->getNumFrames()) next=0; p->setFrame(next); return none())
static PyObject* puttmap_dbase_isCreating(PyObject*,PyObject* args) {
    if (!PyArg_ParseTuple(args,const_cast<char*>(":puttmap_dbase_isCreating"))) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),0);
}
}
PyMethodDef* ssgBindingMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(ssgEntity_getByName) ENTRY(ssgEntity_setName) ENTRY(ssgEntity_getNumKids) ENTRY(ssgEntity_getKid)
        ENTRY(ssgEntity_addKid) ENTRY(ssgEntity_removeKid) ENTRY(ssgEntity_removeAllKids)
        ENTRY(ssgTransform_set) ENTRY(ssgTransform_fastSet) ENTRY(ssgTransform_getPos) ENTRY(ssgTransform_setPos)
        ENTRY(ssgAnim_getNumFrames) ENTRY(ssgAnim_getFrame) ENTRY(ssgAnim_setFrame) ENTRY(ssgAnim_nextFrame)
        ENTRY(puttmap_dbase_isCreating)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
