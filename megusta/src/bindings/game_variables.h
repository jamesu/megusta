// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "Python.h"
namespace megusta { struct DriverViewport; }
namespace megusta::bindings {
// AI_cmd is a process-global command dispatcher; the SWIG receiver is decoded
// for type checking but never dereferenced by the saved native target.
struct AICommandPort {};
extern const AICommandPort aiCommandPort;
// Borrowed native addresses. No dummy game objects are created to make import
// succeed. Set a slot explicitly even when its legitimate value is null.
struct GameVariables {
    const void* objects[21]{};
    bool bound[21]{};
    DriverViewport* viewport=nullptr;
    bool set(const char* name,const void* object);
};
void bindGameVariables(GameVariables*);
bool installGameVariables(PyObject* module);
void resetGameVariables();
}
