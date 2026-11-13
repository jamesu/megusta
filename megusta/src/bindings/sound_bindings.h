// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "Python.h"
namespace megusta { class Sounds; }
namespace megusta::bindings {
void bindSoundServices(Sounds*);
PyMethodDef* soundBindingMethods();
}
