// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "game_shape_adapter.h"
namespace megusta {
// These callbacks identify outstanding native game targets, not successful
// placeholders. The caller must bind actual implementations before loading.
struct GuiShapeServices {
    void* context;
    bool (*load)(void*,GameShapeAdapter&,const char*,unsigned,bool);
    void (*release)(void*,GameShapeAdapter&);
    void (*setSkin)(void*,GameShapeAdapter&,int);
    void (*draw)(void*,GameShapeAdapter&,const float*,const float*,bool);
    bool (*sceneActive)(void*);
};
void bindNativeGuiShapeServices(GuiShapeServices&);
}
