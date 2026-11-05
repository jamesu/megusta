#pragma once
#include "Python.h"
namespace megusta::bindings {
// Explicit registration until recovered application startup calls this family.
PyObject* initializeNetworkModule();
PyMethodDef* networkMethods();
}
