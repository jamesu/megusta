#pragma once
#include "Python.h"
namespace megusta::bindings {
void* decodeReceiver(const char*,const char*,const char*);
template<class T> T* receiver(const char* encoded,const char* tag,const char* function) {
    return static_cast<T*>(decodeReceiver(encoded,tag,function));
}
PyObject* pointerResult(const void*,const char*);
PyObject* none();
}
