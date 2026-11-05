#include "gameplay.h"
#include "runtime.h"
#include "game/scene_triangle.h"
#include "game/random.h"
#include "game/native_game_helpers.h"
#include "engine/opengl_adapter.h"
#include "fnt.h"
#include <cmath>
#include <new>
#include <cstdio>
namespace megusta::bindings {
namespace {
#include "wrapper_macros.h"
#define COORD(C) SWIG_BEGIN(Point3F,C##_set,"sf",float value,&value) p->C=value; \
    return Py_BuildValue(const_cast<char*>("f"),static_cast<double>(value)); } \
    FLOATING(Point3F,C##_get,p->C)
COORD(x) COORD(y) COORD(z)
#undef COORD
static PyObject* new_Point3F(PyObject*,PyObject* args) {
    float x,y,z;
    if (!PyArg_ParseTuple(args,const_cast<char*>("fff:new_Point3F"),&x,&y,&z)) return nullptr;
    return pointerResult(new(std::nothrow) Point3F{x,y,z},"_Point3F_p");
}
static PyObject* delete_Point3F(PyObject*,PyObject* args) {
    char* encoded;
    if (!PyArg_ParseTuple(args,const_cast<char*>("s:delete_Point3F"),&encoded)) return nullptr;
    auto* p=receiver<Point3F>(encoded,"_Point3F_p","delete_Point3F");
    if (!p && PyErr_Occurred()) return nullptr;
    delete p; return none();
}
static long double length(const Point3F& p) {
    // 004d9d50 receives a float argument before performing x87 SQRT.
    const float squared=static_cast<float>(static_cast<long double>(p.z)*p.z+static_cast<long double>(p.y)*p.y+static_cast<long double>(p.x)*p.x);
    return std::sqrt(static_cast<long double>(squared));
}
FLOATING(Point3F,len,length(*p))
SIMPLE(Point3F,normalize,const long double n=length(*p); if (n!=0) {
    const long double inverse=1.0L/n; p->x=static_cast<float>(inverse*p->x);
    p->y=static_cast<float>(inverse*p->y); p->z=static_cast<float>(inverse*p->z);
} return none())
// Type errors for the second pointer must name argument 2, not argument 1.
Point3F* secondPoint(const char* encoded,const char* function) {
    auto* point=receiver<Point3F>(encoded,"_Point3F_p",function);
    if (!point && PyErr_Occurred()) {
        char message[256]; std::sprintf(message,"Type error in argument 2 of %s. Expected _Point3F_p.",function);
        PyErr_SetString(PyExc_TypeError,message);
    }
    return point;
}
#define PAIR(N,...) SWIG_BEGIN(Point3F,N,"ss",char* other,&other) \
    auto* q=secondPoint(other,"Point3F_" #N); if (!q && PyErr_Occurred()) return nullptr; __VA_ARGS__; }
PAIR(dot,return Py_BuildValue(const_cast<char*>("f"),static_cast<double>(
    static_cast<long double>(p->x)*q->x+static_cast<long double>(p->y)*q->y+static_cast<long double>(p->z)*q->z)))
PAIR(dist,const long double x=static_cast<long double>(p->x)-q->x;
    const long double y=static_cast<long double>(p->y)-q->y;
    const long double z=static_cast<long double>(p->z)-q->z;
    const float squared=static_cast<float>(x*x+z*z+y*y);
    return Py_BuildValue(const_cast<char*>("f"),static_cast<double>(std::sqrt(static_cast<long double>(squared)))))
PAIR(add,p->x+=q->x; p->y+=q->y; p->z+=q->z; return none())
PAIR(sub,p->x-=q->x; p->y-=q->y; p->z-=q->z; return none())
#undef PAIR
SWIG_BEGIN(Point3F,scale,"sf",float value,&value)
    // Despite its name, 0041e540 sets the vector's length, rather than simply
    // multiplying its components. Zero vectors remain unchanged.
    const long double n=length(*p);
    if (n!=0) {
        const long double factor=static_cast<long double>(value)/n;
        p->x=static_cast<float>(factor*p->x); p->y=static_cast<float>(factor*p->y); p->z=static_cast<float>(factor*p->z);
    }
    return none(); }
FLOATING(Point3F,getAngle,std::atan2(static_cast<long double>(p->y),static_cast<long double>(p->x))*static_cast<long double>(57.29578f))
SWIG_BEGIN(Point3F,setPolar,"sff",float radius; float degrees,&radius,&degrees)
    const float radians=degrees*0.017453292f;
    p->x=static_cast<float>(std::cos(static_cast<long double>(radians))*radius);
    p->y=static_cast<float>(std::sin(static_cast<long double>(radians))*radius); return none(); }
static PyObject* Rnd_gen(PyObject*,PyObject* args) {
    char* encoded; int limit;
    if (!PyArg_ParseTuple(args,const_cast<char*>("si:Rnd_gen"),&encoded,&limit)) return nullptr;
    auto* p=receiver<Random>(encoded,"_Rnd_p","Rnd_gen"); if (!p && PyErr_Occurred()) return nullptr;
    const unsigned bound=static_cast<unsigned>(limit);
    return Py_BuildValue(const_cast<char*>("i"),bound>1?static_cast<int>(p->generate()%bound):0);
}
static PyObject* generic_isEditing(PyObject*,PyObject* args) {
    if (!PyArg_ParseTuple(args,const_cast<char*>(":generic_isEditing"))) return nullptr;
    return Py_BuildValue(const_cast<char*>("i"),int(isEditing()));
}
SWIG_BEGIN(fntRenderer,start2f,"sff",float x; float y,&x,&y) p->start2f(x,y); return none(); }
SWIG_BEGIN(fntRenderer,start3f,"sfff",float x; float y; float z,&x,&y,&z) p->start3f(x,y,z); return none(); }
SWIG_BEGIN(fntRenderer,setFont,"ss",char* value,&value)
    auto* font=receiver<fntFont>(value,"_fntFont_p","fntRenderer_setFont");
    if (!font && PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError,"Type error in argument 2 of fntRenderer_setFont. Expected _fntFont_p."); return nullptr;
    }
    p->setFont(font); return none(); }
POINTER(fntRenderer,getFont,p->getFont(),fntFont)
SWIG_BEGIN(fntRenderer,setSlant,"sf",float value,&value) p->setSlant(value); return none(); }
SWIG_BEGIN(fntRenderer,setPointSize,"sf",float value,&value) p->setPointSize(value); return none(); }
FLOATING(fntRenderer,getSlant,p->getSlant())
FLOATING(fntRenderer,getPointSize,p->getPointSize())
SIMPLE(fntRenderer,begin,p->begin(); return none())
SIMPLE(fntRenderer,end,p->end(); return none())
SWIG_BEGIN(fntRenderer,putch,"sc",char value,&value) p->putch(value); return none(); }
SWIG_BEGIN(fntRenderer,puts,"ss",char* value,&value) p->puts(value); return none(); }
SWIG_BEGIN(fntRenderer,getStringWidth,"ss",char* text,&text)
    float left,right,bottom,top;
    p->getFont()->getBBox(text,p->getPointSize(),p->getSlant(),&left,&right,&bottom,&top);
    return Py_BuildValue(const_cast<char*>("i"),static_cast<int>(static_cast<long double>(right)-left)); }
SWIG_BEGIN(fntRenderer,setColor,"sffff",float r; float g; float b; float a,&r,&g,&b,&a)
    NEED_GAMEPLAY rendererSetColor(p,r,g,b,a); return none(); }
SWIG_BEGIN(fntRenderer,shadowPuts,"ssii|ffff",char* text; int x; int y; float r=1; float g=1; float b=1; float a=1,&text,&x,&y,&r,&g,&b,&a)
    NEED_GAMEPLAY rendererShadowPuts(p,text,x,y,r,g,b,a); return none(); }
}
PyMethodDef* pointFontMethods() {
    static PyMethodDef methods[]={
#define ENTRY(N) {const_cast<char*>(#N),N,METH_VARARGS,nullptr},
        ENTRY(Point3F_x_set) ENTRY(Point3F_x_get) ENTRY(Point3F_y_set) ENTRY(Point3F_y_get)
        ENTRY(Point3F_z_set) ENTRY(Point3F_z_get) ENTRY(new_Point3F) ENTRY(delete_Point3F)
        ENTRY(Point3F_len) ENTRY(Point3F_normalize) ENTRY(Point3F_dot) ENTRY(Point3F_dist)
        ENTRY(Point3F_add) ENTRY(Point3F_sub) ENTRY(Point3F_scale) ENTRY(Point3F_getAngle) ENTRY(Point3F_setPolar)
        ENTRY(Rnd_gen) ENTRY(generic_isEditing)
        ENTRY(fntRenderer_start2f) ENTRY(fntRenderer_start3f) ENTRY(fntRenderer_setFont) ENTRY(fntRenderer_getFont)
        ENTRY(fntRenderer_setSlant) ENTRY(fntRenderer_setPointSize) ENTRY(fntRenderer_getSlant) ENTRY(fntRenderer_getPointSize)
        ENTRY(fntRenderer_begin) ENTRY(fntRenderer_end) ENTRY(fntRenderer_putch) ENTRY(fntRenderer_puts)
        ENTRY(fntRenderer_getStringWidth) ENTRY(fntRenderer_setColor) ENTRY(fntRenderer_shadowPuts)
#undef ENTRY
        {nullptr,nullptr,0,nullptr}};
    return methods;
}
}
