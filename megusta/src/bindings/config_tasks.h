#pragma once
#include "Python.h"
namespace megusta { class ResourceMountTable; struct TaskRuntime; }
namespace megusta::bindings {
void bindConfigTaskServices(ResourceMountTable*,TaskRuntime*);
PyMethodDef* configTaskMethods();
}
