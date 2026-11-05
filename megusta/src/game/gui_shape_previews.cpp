#include "gui_shape_previews.h"
#include "gui.h"
#include "gui_runtime.h"
#include "gui_render.h"
#include "game_coordinate_system.h"
#include "engine/opengl_adapter.h"
#include "engine/runtime_adapters.h"
#include "engine/string_services.h"
#include "dts/mesh.h"
#include "ssg.h"
#include "ul.h"
#include <cstring>
#include <stdexcept>
namespace megusta {
namespace {
GuiShapeServices& shapeServices() {
    if (!guiRuntime->shapes) throw std::logic_error("GUI shapes require native load/release/skin/render services");
    return *guiRuntime->shapes;
}
void releasePreviewAdapter(GameShapeAdapter*& adapter) {
    if (adapter) { auto& services=shapeServices(); services.release(services.context,*adapter); delete adapter; adapter=nullptr; }
}
}
void guiSystem::loadShapePreview(int slot,const char* path,unsigned flags,unsigned loadFlags) {
    if (!shapePreviewPoolAllocated) {
        for (auto& preview:shapePreviews) preview=GuiShapePreviewRecord{};
        extraShapePreview=nullptr; shapePreviewPoolAllocated=true;
    }
    auto& services=shapeServices();
    const bool displayLists=dts::dtsDisplayListsEnabled,skinLists=dts::dtsSkinDisplayListsEnabled;
    dts::dtsDisplayListsEnabled=false; dts::dtsSkinDisplayListsEnabled=false;
    if (slot<0 || slot>3) engine_runtime::engineRuntime_fatalExit_impl();
    auto& preview=shapePreviews[slot]; releasePreviewAdapter(preview.shapeAdapter); preview=GuiShapePreviewRecord{};
    preview.shapeAdapter=new GameShapeAdapter; preview.shapeAdapter->initialize(); preview.active=true;
    services.load(services.context,*preview.shapeAdapter,path,flags?flags:0x200u,(loadFlags&1)!=0);
    preview.shapeAdapter->advanceTime(0.033333335f,nativeShapeAnimationServices());
    preview.screen[2]=guiRuntime->screenWidth; preview.screen[3]=guiRuntime->screenHeight;
    std::strcpy(preview.displayName,path);
    dts::dtsSkinDisplayListsEnabled=skinLists; dts::dtsDisplayListsEnabled=displayLists;
}
void guiSystem::setShapeSkin(const char* name,int skin) {
    if (!shapePreviewPoolAllocated) return;
    for (auto& preview:shapePreviews) {
        // The recovered loop condition is the shape-adapter pointer itself;
        // a null slot ends the lookup even if later slots contain names.
        if (!preview.shapeAdapter) return;
        if (!strings::compareInsensitive(name,preview.displayName)) {
            auto& services=shapeServices(); services.setSkin(services.context,*preview.shapeAdapter,skin); return;
        }
    }
}
int guiSystem::shapeCommand(int slot,const char* command,int a,int b,int c,int d) {
    if (!shapePreviewPoolAllocated) return 0;
    if (slot<0 || slot>3) engine_runtime::engineRuntime_fatalExit_impl();
    auto& preview=shapePreviews[slot];
    if (!preview.active || !preview.shapeAdapter) {
        ulSetError(UL_WARNING,"dtsCmd(%d,%s,%d,%d,%d) -- Called on inactive slot",
            slot,command,a,b,c);
        return 0;
    }
    const auto is=[&](const char* value){return !strings::compareInsensitive(command,value);};
    // The recovered code converts each integer to float, multiplies by the
    // double literal 0.01, then stores back to float.
    const auto triple=[&](float* values){
        values[0]=static_cast<float>(static_cast<float>(a)*0.01);
        values[1]=static_cast<float>(static_cast<float>(b)*0.01);
        values[2]=static_cast<float>(static_cast<float>(c)*0.01);
    };
    if (is("Loc")) triple(preview.location);
    else if (is("RotPerSec")) triple(preview.rotationPerSecond);
    else if (is("Rot")) triple(preview.rotation);
    else if (is("EyeLoc")) triple(preview.eye);
    else if (is("TargetLoc")) triple(preview.target);
    else if (is("ScreenXYWH")) { preview.screen[0]=a; preview.screen[1]=b; preview.screen[2]=c; preview.screen[3]=d; }
    else if (is("BGColor")) {
        preview.backgroundColour[0]=static_cast<float>(static_cast<float>(a)*0.003921569);
        preview.clearBackground=d!=0;
        preview.backgroundColour[1]=static_cast<float>(static_cast<float>(b)*0.003921569);
        preview.backgroundColour[2]=static_cast<float>(static_cast<float>(c)*0.003921569);
    }
    else if (is("BgBitmapID")) { preview.backgroundAtlas=a; preview.backgroundBitmap=b; }
    else if (is("BGBitmapCoords")) { preview.backgroundRect[0]=a; preview.backgroundRect[1]=b; preview.backgroundRect[2]=c; preview.backgroundRect[3]=d; }
    else if (is("StartSeq")) {
        auto& animation=nativeShapeAnimationServices(); auto& shape=*preview.shapeAdapter;
        shape.playbackFlags&=~1u; shape.setSequence(a,animation); shape.setPosition(0,animation); shape.startPlayback(animation);
    } else if (is("Deactivate")) { releasePreviewAdapter(preview.shapeAdapter); preview.active=false; }
    else ulSetError(UL_WARNING,"dtsCmd(%d,%s,%d,%d,%d) -- Unknown command",
        slot,command,a,b,c);
    return 0;
}
void guiSystem::flushShapePreviews() {
    if (!shapePreviewPoolAllocated) return;
    for (auto& preview:shapePreviews) { releasePreviewAdapter(preview.shapeAdapter); preview.active=false; }
    releasePreviewAdapter(extraShapePreview);
    shapePreviewPoolAllocated=false;
}
void guiSystem::drawShapePreviewBackgrounds() {
    if (!shapePreviewPoolAllocated) return;
    auto& services=shapeServices(); if (services.sceneActive(services.context)) return;
    auto& preview=shapePreviews[0];
    if (!preview.active || !preview.shapeAdapter || !preview.backgroundBitmap) return;
    auto& renderer=*guiRuntime->renderer; renderer.beginTexturedAlpha(1);
    renderer.drawTextureRect(renderer.atlases.resolve(preview.backgroundAtlas,preview.backgroundBitmap),
        preview.backgroundRect[0],preview.backgroundRect[1],preview.backgroundRect[2],preview.backgroundRect[3],false);
    renderer.endTexturedAlpha();
}
void guiSystem::updateShapePreviews() {
    if (!shapePreviewPoolAllocated) return;
    auto& services=shapeServices(); if (services.sceneActive(services.context)) return;
    auto& animation=nativeShapeAnimationServices();
    for (auto& preview:shapePreviews) if (preview.displayName[0] != '\0' && preview.shapeAdapter) {
        auto& shape=*preview.shapeAdapter; shape.advanceTime(0.033333335f,animation);
        for (int i=0;i<3;++i)
            preview.rotation[i]=static_cast<float>(preview.rotationPerSecond[i]*29.999998+preview.rotation[i]);
        if (!(shape.playbackFlags&1)) { shape.setSequence(0,animation); shape.setPosition(0,animation); shape.startPlayback(animation); }
    }
}
void guiSystem::drawShapePreviews() {
    if (!shapePreviewPoolAllocated) return;
    auto& services=shapeServices(); if (services.sceneActive(services.context)) return;
    const bool displayLists=dts::dtsDisplayListsEnabled,skinLists=dts::dtsSkinDisplayListsEnabled;
    dts::dtsDisplayListsEnabled=false; dts::dtsSkinDisplayListsEnabled=false;
    auto& gl=guiRuntime->renderer->gl;
    for (auto& preview:shapePreviews) if (preview.displayName[0] != '\0' && preview.shapeAdapter) {
        gl.enable(0x66); gl.scissor(preview.screen[0],preview.screen[1],preview.screen[2],preview.screen[3]);
        if (preview.clearBackground) { gl.clearColor(preview.backgroundColour[0],preview.backgroundColour[1],preview.backgroundColour[2],1); gl.clearFlags(2); }
        auto* context=ssgGetCurrentContext(); float width,height; sgMat4 savedView,camera,converted,model,basis;
        context->getFOV(&width,&height); context->getModelviewMatrix(savedView); context->setFOV(60,0);
        sgVec3 up={0,0,0.1f}; sgMakeLookAtMat4(camera,preview.eye,preview.target,up); context->setCamera(camera);
        sgTransposeNegateMat4(converted,camera); std::memcpy(basis,gameCoordinateConversionMatrix,sizeof(basis)); sgPostMultMat4(converted,basis);
        sgMakeCoordMat4(model,preview.location,preview.rotation);
        services.draw(services.context,*preview.shapeAdapter,&converted[0][0],&model[0][0],false);
        context->setFOV(width,height); context->setModelviewMatrix(savedView);
    }
    dts::dtsDisplayListsEnabled=displayLists; dts::dtsSkinDisplayListsEnabled=skinLists; gl.disable(0x66);
}
}
