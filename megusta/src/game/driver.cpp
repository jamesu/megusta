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
namespace megusta {
Driver gameDriver{};
DriverViewport driverViewport{};
void DriverViewport::set(int width,int height,bool scale) {
    if (width<0) width=displayWidth;
    viewportWidth=width;
    if (height<0) height=displayHeight;
    viewportHeight=height;
    offsetX=(displayWidth-width)/2; offsetY=(displayHeight-height)/2; scaled=scale;
    if (scale) { offsetX=0; offsetY=0; }
}
void DriverViewport::mapMouse(int& x,int& y) const {
    if (displayWidth>640) {
        if (scaled) { x=(x*viewportWidth)/displayWidth; y=(y*viewportHeight)/displayHeight; y=(y-viewportHeight)+displayHeight; }
        else { x-=offsetX; y+=offsetY; }
    }
}
void DriverViewport::unmapMouse(int& x,int& y) const {
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
    r.disable(r.context,0x4a); r.disable(r.context,0x21); r.disable(r.context,0x18);
    r.disable(r.context,0x67); r.enable(r.context,0x2e); r.blendFunc(r.context,0x37,0x38);
    int width=v.scaled ? v.displayWidth : v.viewportWidth;
    int height=v.scaled ? v.displayHeight : v.viewportHeight;
    r.viewport(r.context,v.offsetX,v.offsetY,width,height); r.matrixMode(r.context,0xf); r.loadIdentity(r.context);
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
    setup3dRenderState(r); s.hole.draw(s.holeServices); s.gui.drawRoot();
    networkServiceEnd(); setup2dProjection(s.viewport,r);
    if (fpsOverlayEnabled) drawPerformanceOverlay(guiFonts.primary,s.clock,s.viewport.displayWidth,
        s.viewport.displayHeight,s.client&&s.client->inGame());
    s.hole.drawHud(s.holeServices); s.displayWidgets(s.context,0,9999);
    if(s.client)drawGuiChatOverlay(*s.client,s.holeServices.runtime.objects.sceneRoot!=nullptr,
        gameDriver.activeScript,s.guiDebug);
    s.holeServices.runtime.tasks.scheduledTasks.runDrawCallbacks(s.holeServices.runtime.tasks);
    drawGuiTooltip(); s.console.drawOverlay(); flushGuiScenePreview(); networkServiceEnd();
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
