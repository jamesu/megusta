#include "gui_assets.h"
#include "fnt.h"
#include "engine/runtime_adapters.h"
#include <GL/glpng.h>
#include <cstring>
#include <new>
#include <stdexcept>
namespace megusta {
GuiAssetServices guiAssetServices(ResourceMountTable& mounts) {
    return {&mounts,[](void* context,const char* path,bool external,int mipmaps,int wrap,int min,int mag) {
        return pix::loadMultiTexture(*static_cast<ResourceMountTable*>(context),path,external,mipmaps,wrap,min,mag);
    }};
}
namespace {
GuiTextureAtlas* loadNine(const GuiAssetServices& s,const char* path) {
    if (!s.loadAtlas) throw std::logic_error("GUI atlas loader is not bound");
    auto* atlas=s.loadAtlas(s.context,path,true,0,GL_CLAMP,GL_NEAREST,GL_NEAREST);
    if (!atlas || atlas->textureCount!=9) engine_runtime::engineRuntime_fatalExit_impl();
    return atlas;
}
}
// 00403810: deliberately no initialized guard in this body.
void GuiWidgetResources::loadToggleLozenge(const GuiAssetServices& s) {
    toggleInitialized=true;
    toggleAtlas=loadNine(s,"data/textures/gui/LOZG.mpx");
    toggleFont=new(std::nothrow) fntTexFont("data/fonts/adela24.txf",GL_LINEAR,GL_LINEAR);
    togglePointSize=23;
}
// 004043d0 rejects a second initialization rather than silently reusing it.
void GuiWidgetResources::loadLozenge(const GuiAssetServices& s) {
    if (lozengeInitialized) engine_runtime::engineRuntime_fatalExit_impl();
    lozengeInitialized=true;
    lozengeAtlas=loadNine(s,"data/textures/gui/LOZLR.mpx");
}
// 00404860, also intentionally unguarded.
void GuiWidgetResources::loadHeaderFont() {
    headerInitialized=true;
    headerFont=new(std::nothrow) fntTexFont("data/fonts/adela42.txf",GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);
    headerPointSize=40;
}
void GuiWidgetResources::markTextInitialized() { textInitialized=true; } // 00405630
void GuiWidgetResources::markSimpleButtonInitialized() { simpleButtonInitialized=true; } // 00406160
void GuiWidgetResources::loadShadowAtlas(const GuiAssetServices& s) { // 00404920
    if (!shadowAtlas) shadowAtlas=loadNine(s,"data/textures/gui/SHADBX.mpx");
}
void GuiWidgetResources::loadAll(const GuiAssetServices& s) { // 0040cbf0
    loadToggleLozenge(s);
    loadLozenge(s);
    loadHeaderFont();
    markTextInitialized();
    markSimpleButtonInitialized();
    loadShadowAtlas(s);
}
bool loadGuiPngTexture(std::uint32_t& name,const char* path) { // 004029c0
    static pngInfo info; // Original shared scratch at 00525a48.
    std::memset(&info,0,16);
    name=pix::bindPngTexture(path,PNG_NOMIPMAPS,GL_CLAMP,GL_NEAREST,GL_NEAREST);
    return true;
}
bool loadGuiPngTexture(ResourceMountTable& mounts,std::uint32_t& name,const char* path) {
    name=pix::bindPngTexture(mounts,path,PNG_NOMIPMAPS,GL_CLAMP,GL_NEAREST,GL_NEAREST);
    return true;
}
}
