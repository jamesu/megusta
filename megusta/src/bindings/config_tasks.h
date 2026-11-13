// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "Python.h"
namespace megusta { class ResourceMountTable; struct TaskRuntime; }
namespace megusta::bindings {
void bindConfigTaskServices(ResourceMountTable*,TaskRuntime*);
PyMethodDef* configTaskMethods();
}
