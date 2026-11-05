#pragma once
#include "Python.h"
namespace megusta {
struct PlayerServices; struct BallRestServices; struct BallGameplayServices;
struct SceneObjectCollection; class PuttMap; class PuttMapRuntime; class OpenGLAdapter;
}
namespace megusta::bindings {
struct GameplayBindingServices {
    const PlayerServices& players;
    const BallRestServices& rest;
    const BallGameplayServices& ball;
    SceneObjectCollection& objects;
    PuttMap& puttMap;
    OpenGLAdapter& gl;
    int& editorMode;
    unsigned& lastUsedPowerUpId;
    PuttMapRuntime* scriptRuntime=nullptr;
};
void bindGameplayServices(GameplayBindingServices*);
GameplayBindingServices* gameplayServices();
PyObject* missingGameplayServices();
PyMethodDef* pointFontMethods();
PyMethodDef* playerBallMethods();
}
