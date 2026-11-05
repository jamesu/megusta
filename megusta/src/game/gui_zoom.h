#pragma once
#include "scene_runtime.h"
#include <cstdint>
namespace megusta {
class guiSystem;
struct GuiZoomAsset { char* name; ssgEntity* entity; ssgAnim* animation; GuiZoomAsset* next; };
struct GuiZoomSlot { GuiZoomAsset* asset=nullptr; int state=0; std::uint32_t holdTicks=0,holdStartTick=0,remainingHoldTicks=0; };
class GuiMaterialNameProvider : public SceneModelOptions {
public:
    const char* const* materialNames=nullptr;
    int materialCount=0;
    ssgSimpleState* createSimpleState(char*,int) const override;
};
void preloadGuiZoom(const char*,const char*,const char*,const char*);
}
