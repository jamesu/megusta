#pragma once
#include <cstdint>
#include "engine/multi_texture.h"
class fntTexFont;
namespace megusta {
// Normalized counterpart of the original count followed by texture-name array.
// Ownership of GL names belongs to the atlas loader/runtime, not each widget.
using GuiTextureAtlas=pix::MultiTextureList;
struct GuiAssetServices {
    void* context;
    GuiTextureAtlas* (*loadAtlas)(void*,const char*,bool,int,int,int,int);
};
GuiAssetServices guiAssetServices(ResourceMountTable&);
class GuiWidgetResources {
public:
    bool toggleInitialized=false,lozengeInitialized=false,headerInitialized=false;
    bool textInitialized=false,simpleButtonInitialized=false;
    GuiTextureAtlas *toggleAtlas=nullptr,*lozengeAtlas=nullptr,*shadowAtlas=nullptr;
    fntTexFont *toggleFont=nullptr,*headerFont=nullptr;
    int togglePointSize=0,headerPointSize=0;
    void loadAll(const GuiAssetServices&);
    void loadToggleLozenge(const GuiAssetServices&);
    void loadLozenge(const GuiAssetServices&);
    void loadHeaderFont();
    void markTextInitialized();
    void markSimpleButtonInitialized();
    void loadShadowAtlas(const GuiAssetServices&);
};
// 004029c0 always returns true, including a zero texture name from pngBind.
bool loadGuiPngTexture(std::uint32_t& name,const char* path);
bool loadGuiPngTexture(ResourceMountTable&,std::uint32_t& name,const char* path);
}
