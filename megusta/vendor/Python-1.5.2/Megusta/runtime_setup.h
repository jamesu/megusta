#pragma once
#include "Python.h"
namespace megusta {
// Driver_initialize_impl 00401e40 sets these three words immediately before
// Py_Initialize: 0053de2c=1, 0054f55c=1, 005056d8=0. This is application
// startup policy, not a modification of CPython's stock global initializers.
inline void configureGamePython(bool classExceptions=false) {
    Py_OptimizeFlag=1;
    Py_NoSiteFlag=1;
    Py_UseClassExceptionsFlag=classExceptions?1:0;
}
}
