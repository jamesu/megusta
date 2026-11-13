// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "hole.h"
#include "../engine/opengl_adapter.h"
namespace megusta {
HoleRenderServices makeOpenGLHoleRenderServices(OpenGLAdapter& adapter);
}
