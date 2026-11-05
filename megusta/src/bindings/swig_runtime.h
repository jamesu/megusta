#pragma once

#include "Python.h"
#include <cstddef>
#include <cstdio>

namespace megusta::bindings {
using SwigGetter = PyObject* (*)();
using SwigSetter = int (*)(PyObject*);
using SwigCast = void* (*)(void*);

// The saved 32-bit records are a Python object header followed by the
// null-terminated variable pointer array, count and capacity. Native builds
// use the host pointer width while preserving that field order.
struct SwigVariable {
    char* name;
    SwigGetter getter;
    SwigSetter setter;
};
struct SwigVarLink {
    PyObject_HEAD
    SwigVariable** variables;
    int count;
    int capacity;
};
struct SwigTypeMapping {
    char* name;
    int nameLength;
    SwigCast cast;
    SwigTypeMapping* next;
};

SwigVarLink* swig_varlink_new();
void swig_varlink_add(SwigVarLink*, const char*, SwigGetter, SwigSetter);
void swig_varlink_destroy(SwigVarLink*);
void swig_varlink_dealloc(PyObject*);
PyObject* swig_varlink_repr(PyObject*);
int swig_varlink_print(PyObject*, FILE*, int);
PyObject* swig_varlink_getattr(PyObject*, char*);
int swig_varlink_setattr(PyObject*, char*, PyObject*);
void swig_track_varlink(SwigVarLink** owner, SwigVarLink* link);
void swig_register_type_mapping(const char*, const char*, SwigCast);
void swig_encode_pointer(char*, const void*, const char*);
char* swig_decode_pointer(const char*, void**, const char*);
int swig_compare_mapping_names(const void*, const void*);
int swig_compare_type_prefix(const char*, const SwigTypeMapping*);
void* swig_identity_upcast(void*);

// Lifecycle entrypoints corresponding to the two saved SWIG initializers.
void swig_init_network_bindings(bool reload);
void swig_init_gamec_bindings(bool reload);
}
