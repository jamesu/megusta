#pragma once
#include "platform/surface_transform.h"
namespace megusta::platform {
void clearLetterboxBars(const SurfaceTransform&,int canvasWidth,int canvasHeight);
void beginWorldOverlay(const SurfaceTransform&,int canvasWidth,int canvasHeight);
void endWorldOverlay();
}
