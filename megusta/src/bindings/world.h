// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "Python.h"
#include <cstdint>
class ssgEntity;
namespace megusta {
class Driver; class Camera; class Putter;
struct CameraRuntime; struct CameraServices; struct DriverScriptServices;struct HoleServices;
struct PutterAnimationServices; struct SceneObjectAnimationServices; struct SceneObjectCollection;
struct PutterStrikeServices; struct PutterTuning;
}
namespace megusta::bindings {
struct CameraBindingServices {
    CameraRuntime& state;
    std::uint32_t& fixedTick;
    int& collisionFilter;
    void (*showCollisionMessage)(const char*);
};
void bindCameraServices(CameraBindingServices*);
// Native globals/services needed by these wrappers. Unrecovered target bodies
// are explicit callable dependencies, never silent successful placeholders.
struct WorldBindingServices {
    SceneObjectAnimationServices& animation;
    CameraRuntime& camera;
    CameraServices& cameraServices;
    PutterAnimationServices& putterAnimation;
    const PutterStrikeServices& strike;
    const PutterTuning& tuning;
    float& cameraHeading;
    DriverScriptServices& scripts;
    Driver& driver;
    std::uint32_t& fixedTick;
    int& collisionFilter;
    bool& demo;
    void (*showCollisionMessage)(const char*);
    void (*wireDebug)();
    void (*registerJoystick)(bool);
    void (*cameraDump)(Camera&);
    void (*putterDump)(Putter&);
};
void bindWorldServices(WorldBindingServices*);
void bindDriverServices(DriverScriptServices*,void (*registerJoystick)(bool));
HoleServices* boundHoleServices();
void bindSceneAnimationServices(SceneObjectAnimationServices*);
SceneObjectAnimationServices* sceneAnimationServices();
WorldBindingServices* worldServices();
PyObject* missingWorldServices();
PyMethodDef* ssgBindingMethods();
PyMethodDef* objectPlayerMethods();
PyMethodDef* cameraPutterDriverMethods();
}
