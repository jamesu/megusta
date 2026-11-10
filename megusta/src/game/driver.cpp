#include "driver.h"
#include "sounds.h"
#include "camera.h"
#include "room_client.h"
#include "gui.h"
#include "gui_runtime.h"
#include "gui_shell.h"
#include "gui_tooltip.h"
#include "gui_widgets.h"
#include "hud.h"
#include "console.h"
#include "pu.h"
#include "ssg.h"
#ifdef MEGUSTA_SDL
#include "platform/sdl/presentation.h"
#endif
namespace megusta {
Driver gameDriver{};
DriverViewport driverViewport{};
platform::PixelRectangle DriverViewport::pixelRectangle()const {
    if(surface.enabled())return surface.fit(viewportWidth,viewportHeight);
    return {offsetX,offsetY,scaled?displayWidth:viewportWidth,scaled?displayHeight:viewportHeight};
}
platform::PixelRectangle DriverViewport::pixelScissor(int x,int y,int width,int height)const {
    if(surface.enabled())return surface.toPixels(x,y,width,height,viewportWidth,viewportHeight);
    return {x,y,width,height};
}
void DriverViewport::set(int width,int height,bool scale) {
    if (width<0) width=displayWidth;
    viewportWidth=width;
    if (height<0) height=displayHeight;
    viewportHeight=height;
    offsetX=(displayWidth-width)/2; offsetY=(displayHeight-height)/2; scaled=scale;
    if (scale) { offsetX=0; offsetY=0; }
}
void DriverViewport::mapMouse(int& x,int& y) const {
    if(surface.enabled()) {
        surface.toCanvas(x,y,viewportWidth,viewportHeight);
        y+=displayHeight-viewportHeight;return;
    }
    if (displayWidth>640) {
        if (scaled) { x=(x*viewportWidth)/displayWidth; y=(y*viewportHeight)/displayHeight; y=(y-viewportHeight)+displayHeight; }
        else { x-=offsetX; y+=offsetY; }
    }
}
void DriverViewport::unmapMouse(int& x,int& y) const {
    if(surface.enabled()) {
        y-=displayHeight-viewportHeight;
        surface.toWindow(x,y,viewportWidth,viewportHeight);return;
    }
    // Platform cursor warps take client coordinates. GUI callers supply the
    // coordinates produced by mapMouse, including its display-height offset.
    if(displayWidth>640) {
        if(scaled) { x=(x*displayWidth)/viewportWidth;y=((y+viewportHeight-displayHeight)*displayHeight)/viewportHeight; }
        else { x+=offsetX;y-=offsetY; }
    }
}
void Driver::setup3dRenderState(HoleRenderServices& r) {
    r.shadeModel(r.context,0x3d); r.disable(r.context,0x4a); r.enable(r.context,0x21);
    r.enable(r.context,0x18); r.disable(r.context,0x67); r.frontFace(r.context,0x13);
    r.depthFunc(r.context,0x27); r.disable(r.context,0x2e); r.blendFunc(r.context,0x32,0x31);
}
void Driver::setup2dProjection(DriverViewport& v,HoleRenderServices& r) {
    r.selectBaseTextureUnit(r.context);
    r.disable(r.context,0x4a); r.disable(r.context,0x21); r.disable(r.context,0x18);
    r.disable(r.context,0x67); r.disable(r.context,0x66);
    r.enable(r.context,0x2e); r.blendFunc(r.context,0x37,0x38);
    // DTS may leave a texture-combine environment on unit zero; PLIB's
    // fntTexFont assumes the fixed-function default modulation for its atlas.
    r.texEnvi(r.context,0x48,0x49,0x56);
    r.colorMask(r.context,true,true,true,true); r.depthMask(r.context,true);
    r.depthRange(r.context,0.0,1.0);
    const auto rectangle=v.pixelRectangle();
    r.viewport(r.context,rectangle.x,rectangle.y,rectangle.width,rectangle.height); r.matrixMode(r.context,0xf); r.loadIdentity(r.context);
    r.ortho(r.context,0.0,static_cast<float>(v.viewportWidth),0.0,static_cast<float>(v.viewportHeight),0.0,1.0);
    r.matrixMode(r.context,0xe); r.loadIdentity(r.context);
}
void Driver::frame(DriverServices& s) {
    HoleRenderServices& r=s.holeServices.render;
    if (clearNextFrame) { r.clearColor(r.context,0,0,0,1); r.clearFlags(r.context,3); clearNextFrame=false; }
    applyPendingScript(s.script);
    s.guiState.screenWidth=s.viewport.displayWidth;
    s.guiState.screenHeight=s.viewport.displayHeight;
    s.gui.updateFrameDelta();
    if (s.hasSoftwareCursor(s.context)) {
        if (!s.windowCursorEnabled(s.context) || !s.hasTabElements(s.context)) puHideCursor();
        else puShowCursor();
    }
    if (s.shellOpen(s.context) || s.hasBackground(s.context)) {
        setup2dProjection(s.viewport,r); r.enable(r.context,0x21); r.clearColor(r.context,0,0,0,1);
        r.clearFlags(r.context,3); r.disable(r.context,0x21); s.gui.drawShellBackground();
        s.displayWidgets(s.context,-9999,-1); networkServiceEnd();
    }
    const auto guiRectangle=s.viewport.pixelRectangle();
    s.viewport.worldExpanded=s.viewport.surface.enabled() && s.viewport.expandGameView &&
        s.hole.hasActiveScene(s.holeServices.runtime.objects);
    auto sceneRectangle=guiRectangle;
    auto* sceneContext=ssgGetCurrentContext();float originalHorizontal,originalVertical;
    sceneContext->getFOV(&originalHorizontal,&originalVertical);
    if(s.viewport.worldExpanded) {
        sceneRectangle={0,0,s.viewport.surface.drawableWidth,s.viewport.surface.drawableHeight};
        float horizontal=originalHorizontal,vertical=originalVertical;
        s.viewport.surface.expandedFov(horizontal,vertical,s.viewport.viewportWidth,s.viewport.viewportHeight);
        sceneContext->setFOV(horizontal,vertical);
    }
    r.viewport(r.context,sceneRectangle.x,sceneRectangle.y,sceneRectangle.width,sceneRectangle.height);
    setup3dRenderState(r); s.hole.draw(s.holeServices);
    if(s.viewport.worldExpanded)sceneContext->setFOV(originalHorizontal,originalVertical);
    r.viewport(r.context,guiRectangle.x,guiRectangle.y,guiRectangle.width,guiRectangle.height);
    s.gui.drawRoot();
    networkServiceEnd(); setup2dProjection(s.viewport,r);
    if (fpsOverlayEnabled) drawPerformanceOverlay(guiFonts.primary,s.clock,s.viewport.displayWidth,
        s.viewport.displayHeight,s.client&&s.client->inGame());
    s.hole.drawHud(s.holeServices); s.displayWidgets(s.context,0,9999);
    if(s.client)drawGuiChatOverlay(*s.client,s.holeServices.runtime.objects.sceneRoot!=nullptr,
        gameDriver.activeScript,s.guiDebug);
#ifdef __EMSCRIPTEN__
    // Widgets and overlays have their own texture/blend state. The scripted
    // HUD font pass needs the same 2D baseline as the native HUD pass.
    setup2dProjection(s.viewport,r);
#endif
    s.holeServices.runtime.tasks.scheduledTasks.runDrawCallbacks(s.holeServices.runtime.tasks);
    drawGuiTooltip(); s.console.drawOverlay(); flushGuiScenePreview(); networkServiceEnd();
#ifdef MEGUSTA_SDL
    if(!s.viewport.worldExpanded)platform::clearLetterboxBars(s.viewport.surface,s.viewport.viewportWidth,s.viewport.viewportHeight);
#endif
    networkServiceEnd();
}
void Driver::idleUpdate(DriverServices& s) {
    s.holeServices.inputState.readFrame(s.hole.hasActiveScene(s.holeServices.runtime.objects));
    const auto& eye=s.audioCamera.controlEye;
    const auto& look=s.audioCamera.controlLookAt;
    float position[3]={eye.x,eye.y,eye.z},target[3]={look.x,look.y,look.z};
    s.sounds.updateListener(s.hole.hasActiveScene(s.holeServices.runtime.objects),position,target);
    s.gui.updateInteraction(s.holeServices.config.debugKeys(),s.client&&s.client->inGame(),s.guiDebug);
    auto& runtime=s.holeServices.runtime;
    if (!s.slowMotionTickDivider) { s.hole.update(s.clock,s.holeServices); runtime.tasks.scheduledTasks.runIdleCallbacks(runtime.tasks); }
    if(s.client)s.client->pollDisconnectAndTurnState();
    s.clock.sampleFrameRates(s.timeMilliseconds(s.context));
    bool ready=s.coordinateTurnReadiness(s.context);
    while (ready && (s.clock.advanceFixedTick(s.timeMilliseconds(s.context)) || s.cameraCollisionEnabled(s.context))) {
        s.gui.advanceZoomAnimations();
        if (s.slowMotion && ++s.slowMotionTickDivider>5) s.slowMotionTickDivider=0;
        if (!s.slowMotionTickDivider) { s.hole.processFrame(s.clock,s.holeServices); runtime.tasks.scheduledTasks.tickScheduled(runtime.tasks); }
        if(s.client)s.client->processTurnSync();
        s.script.autorun.update(false,*this,s.script,s.script.autorunServices);
        ready=s.coordinateTurnReadiness(s.context);
    }
    s.dispatchJoystick(s.context); networkServiceEnd();
    std::uint32_t now=previousIdleTime;
    if (frameLimitEnabled) {
        now=s.timeMilliseconds(s.context);
        if (now-previousIdleTime<33u) { s.sleepMicroseconds(s.context,(33u-(now-previousIdleTime))*1000u); now=s.timeMilliseconds(s.context); }
    }
    previousIdleTime=now;
}
}
