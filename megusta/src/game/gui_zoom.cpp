#include "gui_zoom.h"
#include "gui.h"
#include "gui_runtime.h"
#include "ssgGameAnimation.h"
#include "engine/string_services.h"
#include "engine/opengl_adapter.h"
#include "gui_render.h"
#include "../engine/platform_gl.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
namespace megusta {
namespace { GuiZoomAsset* zoomAssets=nullptr; GuiZoomSlot zoomSlots[4]; }
ssgSimpleState* GuiMaterialNameProvider::createSimpleState(char*,int material) const {
    if (!materialNames || material<0 || material>=materialCount || !materialNames[material] || !*materialNames[material]) return nullptr;
    auto* state=new ssgSimpleState;
    const std::string name=std::string(materialNames[material])+".png";
    auto* texture=ssgGetCurrentOptions()->createTexture(const_cast<char*>(name.c_str()),1,1,1);
    state->setTexture(texture); state->enable(GL_TEXTURE_2D);
    if (texture->hasAlpha()) { state->enable(GL_BLEND); state->setTranslucent(); }
    else { state->disable(GL_BLEND); state->setOpaque(); }
    state->disable(GL_LIGHTING); state->setShadeModel(GL_SMOOTH);
    return state;
}
void preloadGuiZoom(const char* name,const char* filename,const char* first,const char* second) {
    if (!name || !filename) return;
    for (auto* asset=zoomAssets;asset;asset=asset->next) if (!std::strcmp(name,asset->name)) return;
    const char* materials[2]={first,second}; GuiMaterialNameProvider provider;
    provider.materialNames=materials; provider.materialCount=2; provider.enabled=true;
    auto* entity=loadSceneModel(filename,true,&provider);
    if (entity) {
        auto* asset=new GuiZoomAsset{strings::duplicate(name),entity,nullptr,zoomAssets};
        entity->ref(); asset->animation=createSceneAnimation(entity); asset->animation->ref();
        asset->animation->setFrame(1); zoomAssets=asset;
    }
}
void guiSystem::playZoom(const char* name,std::uint32_t hold) {
    if (!name) return;
    auto* asset=zoomAssets;
    while (asset && std::strcmp(name,asset->name)) asset=asset->next;
    if (!asset) return;
    for (int i=0;i<4;++i) if (!zoomSlots[i].asset) {
        auto& slot=zoomSlots[i]; slot.asset=asset; slot.state=0; slot.holdTicks=hold;
        if (i<3) getRoot()->addKid(asset->entity);
        return;
    }
}
void guiSystem::resetZoomAnimations() {
    for (auto& slot:zoomSlots) if (slot.asset) {
        if (previewRoot) previewRoot->removeKid(slot.asset->entity);
        slot.asset->animation->setFrame(1); slot.asset=nullptr;
    }
}
void guiSystem::releaseZoomCache() {
    resetZoomAnimations();
    while (zoomAssets) {
        auto* asset=zoomAssets; zoomAssets=asset->next;
        std::free(asset->name); ssgDeRefDelete(asset->entity); ssgDeRefDelete(asset->animation); delete asset;
    }
}
void guiSystem::advanceZoomAnimations() {
    for (auto& slot:zoomSlots) {
        const auto tick=guiRuntime->fixedTick(guiRuntime->context);
        auto* asset=slot.asset; if (!asset) continue;
        if (!slot.state) {
            auto* animation=asset->animation;
            if (animation->getFrame()<animation->getNumFrames()-1) animation->setFrame(animation->getFrame()+1);
            else { slot.state=1; slot.holdStartTick=tick; slot.remainingHoldTicks=slot.holdTicks; }
        } else if (slot.state==1 && slot.remainingHoldTicks<=tick-slot.holdStartTick) {
            if (previewRoot) previewRoot->removeKid(asset->entity);
            asset->animation->setFrame(1); slot.asset=nullptr;
        }
    }
    updateShapePreviews();
}
void guiSystem::drawRoot() {
    advanceShellBackground(); drawShapePreviews();
    if (previewRoot) {
        auto* context=ssgGetCurrentContext(); float width,height; sgMat4 view;
        context->getFOV(&width,&height); context->getModelviewMatrix(view);
        sgCoord origin={{0,0,0},{0,0,0}}; context->setCamera(&origin); context->setFOV(45.000103f,0);
        guiRuntime->renderer->gl.clearFlags(1); ssgCullAndDraw(previewRoot);
        context->setFOV(width,height); context->setModelviewMatrix(view);
    }
}
}
