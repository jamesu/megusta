// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "Python.h"
namespace megusta::bindings {
PyMethodDef* basicGameMethods();
// Registers only the implemented subset; unimplemented entries remain absent.
PyObject* initializeBasicGameModule();
}
