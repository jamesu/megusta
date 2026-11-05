#include "hole.h"
#include "putt_map_runtime.h"
#include "../engine/string_services.h"
#include <cstdlib>
namespace megusta {
Hole gameHole{};
HoleFrameClock gameHoleClock{};
void Hole_global_initialize_impl() { gameHole.clearGlobalPrefix(); }
void HoleFrameClock::resetFrameCounters(std::uint32_t now) {
    totalFixedTickCount=0; nextFixedTickDeadlineMs=now; rateWindowStartMs=0; previousFrameCount=0; previousTickCount=0;
}
void HoleFrameClock::sampleFrameRates(std::uint32_t now) {
    if (rateWindowStartMs+2000u<now) {
        std::uint32_t elapsed=now-rateWindowStartMs, frames=totalRenderedFrameCount-previousFrameCount;
        std::uint32_t ticks=totalFixedTickCount-previousTickCount;
        previousFrameCount=totalRenderedFrameCount; fixedTicksThisFrame=0; rateWindowStartMs=now;
        previousTickCount=totalFixedTickCount; ++totalRenderedFrameCount;
        framesPerSecond=(static_cast<float>(frames)*1000.0)/static_cast<float>(elapsed);
        fixedTicksPerSecond=(static_cast<float>(ticks)*1000.0)/static_cast<float>(elapsed); return;
    }
    fixedTicksThisFrame=0; ++totalRenderedFrameCount;
}
void HoleFrameClock::resetFrameDeadline(std::uint32_t now) { nextFixedTickDeadlineMs=now; fixedTicksThisFrame=0; }
bool HoleFrameClock::advanceFixedTick(std::uint32_t now) {
    if (nextFixedTickDeadlineMs<now) {
        if (fixedTicksThisFrame<6) { nextFixedTickDeadlineMs+=0x21; ++totalFixedTickCount; ++fixedTicksThisFrame; return true; }
    } else { now+=0x21; if (nextFixedTickDeadlineMs<=now) { fixedTicksThisFrame=0; return false; } }
    nextFixedTickDeadlineMs=now; fixedTicksThisFrame=0; return false;
}
void Hole::clearGlobalPrefix() { ownedInitString0=nullptr; ownedInitString1=nullptr; }
bool Hole::hasActiveScene(const SceneObjectCollection& objects) const { return objects.sceneRoot!=nullptr; }
bool Hole::isTransitioning(const SceneObjectCollection& objects) const {
    return hasActiveScene(objects) && transitionState!=2 && transitionState!=3;
}
void Hole::beginTransition(HoleFrameClock& clock) {
    transitionState=2; clock.transitionStartTick=clock.totalFixedTickCount; clock.transitionDurationTicks=60;
}
void Hole::releaseResources(HoleServices& s) {
    s.setAmbientColour(s.context,1,1,1);
    if (!hasActiveScene(s.runtime.objects)) return;
    s.action(s.context,HoleAction::releasePuttMap); s.action(s.context,HoleAction::saveSpatialObjects);
    s.runtime.objects.releaseSceneRecords(); s.action(s.context,HoleAction::releaseCamera);
    s.action(s.context,HoleAction::releaseBallModels); s.action(s.context,HoleAction::releasePutter);
    s.runtime.players.release(s.runtime.host.playerServices); s.action(s.context,HoleAction::releaseTransientSounds);
    s.action(s.context,HoleAction::releaseEffects); s.action(s.context,HoleAction::serviceNetwork);
    s.action(s.context,HoleAction::serviceNetwork); s.runtime.grid.releaseSceneIndex();
    s.action(s.context,HoleAction::releasePowerUps); gameHole.transitionState=0;
    std::free(ownedInitString0); ownedInitString0=nullptr; std::free(ownedInitString1); ownedInitString1=nullptr;
}
void Hole::initialize(const char* first,const char* second,int argument,HoleFrameClock& clock,HoleServices& s) {
    if (hasActiveScene(s.runtime.objects)) return;
    std::free(ownedInitString0); ownedInitString0=nullptr; std::free(ownedInitString1); ownedInitString1=nullptr;
    ownedInitString0=strings::duplicate(first); ownedInitString1=strings::duplicate(second); initIntegerArgument=argument;
    s.inputState.initialize(); transitionState=1;
    s.action(s.context,HoleAction::initializeSceneRoot); s.action(s.context,HoleAction::buildSceneObjects);
    s.runtime.grid.buildSceneIndex(s.runtime.objects); s.action(s.context,HoleAction::releaseTransientSounds);
    s.action(s.context,HoleAction::loadBallModels);
    s.runtime.players.initializeHole(clock.totalFixedTickCount,s.runtime.balls,s.runtime.tasks,s.runtime.host.playerServices);
    s.action(s.context,HoleAction::loadPutter); s.action(s.context,HoleAction::initializeCamera);
    s.action(s.context,HoleAction::initializeEffects); s.action(s.context,HoleAction::serviceNetwork);
    s.action(s.context,HoleAction::initializeWireDebug); s.action(s.context,HoleAction::loadPowerUps);
    SceneObjectRecord* tee=s.runtime.objects.findByName("tee_01");
    const float* position=tee->getPosition();
    for (int i=0;i<3;++i) teePositions[0][i]=position[i];
    auto triangleServices=s.runtime.objects.triangleServices(); Point3F vertices[3];
    for (int i=0;i<2;++i) {
        tee->triangles[i].copyVertices(vertices,triangleServices);
        teePositions[i+1][2]=teePositions[0][2];
        teePositions[i+1][0]=((vertices[2].x+vertices[1].x)*0.5+teePositions[0][0])*0.5;
        teePositions[i+1][1]=((vertices[2].y+vertices[1].y)*0.5+teePositions[0][1])*0.5;
    }
    s.action(s.context,HoleAction::loadSpatialObjects);
    s.action(s.context,HoleAction::loadPuttMapBinary);
    s.action(s.context,HoleAction::loadPuttMapScript);
}
void Hole::update(HoleFrameClock& clock,HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) return;
    if (transitionState==1) {
        PlayerInputContext inputContext{s.input,teePositions,s.smoothedAim,s.runtime.balls,s.runtime.tasks,s.runtime.rest,s.runtime.host.playerServices};
        s.runtime.players.handleInput(inputContext);
    } else if (transitionState==2) {
        if (clock.transitionDurationTicks<=clock.totalFixedTickCount-clock.transitionStartTick) transitionState=3;
    } else if (transitionState==3) s.runtime.players.startNextHoleTask(s.runtime.tasks,s.runtime.host.playerServices);
    if (!s.isEditing(s.context)) s.action(s.context,HoleAction::putterInput);
    s.action(s.context,HoleAction::wireInput); s.action(s.context,HoleAction::powerUpInput);
    s.action(s.context,HoleAction::cameraInput); s.action(s.context,HoleAction::spatialEditorInput);
}
void Hole::processFrame(HoleFrameClock& clock,HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) return;
    s.runtime.stepBalls(); if (!s.isEditing(s.context)) s.action(s.context,HoleAction::advancePutter);
    s.runtime.players.advanceTurnIfReady(clock.totalFixedTickCount,s.runtime.balls,s.runtime.host.playerServices);
    s.action(s.context,HoleAction::updateEffects); s.action(s.context,HoleAction::serviceNetwork);
    s.action(s.context,HoleAction::processPickups); s.action(s.context,HoleAction::applyCameraInput);
}
void setupHoleHudRenderState(HoleRenderServices& r) {
    r.matrixMode(r.context,0xf); r.loadIdentity(r.context); r.projectionIdentity(r.context);
    r.matrixMode(r.context,0xe); r.loadIdentity(r.context); r.depthRange(r.context,0.0,1.0);
    r.colorMask(r.context,true,true,true,true); r.disable(r.context,0x66); r.polygonMode(r.context,0x15,0x12);
    r.enable(r.context,0x18); r.frontFace(r.context,0x13);
    r.disableClientState(r.context,0x1b); r.disableClientState(r.context,0x1d); r.disableClientState(r.context,0x1e);
    r.blendFunc(r.context,0x37,0x38); r.texEnvi(r.context,0x48,0x49,0x56);
    r.disable(r.context,0x4a); r.disable(r.context,0x2e); r.depthMask(r.context,true);
    r.disable(r.context,0x23); r.disable(r.context,0x67); r.color4f(r.context,1,1,1,1); r.shadeModel(r.context,0x3d);
}
void Hole::draw(HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) { s.action(s.context,HoleAction::drawSceneRoot); return; }
    s.action(s.context,HoleAction::drawBalls); s.action(s.context,HoleAction::updateCameraFollow);
    s.action(s.context,HoleAction::drawAimPreview); s.action(s.context,HoleAction::collectWireCandidates);
    s.render.clearColor(s.render.context,0.25f,0.25f,0.25f,1.0f); s.render.clearFlags(s.render.context,3);
    setupHoleHudRenderState(s.render);
    if (!s.isEditing(s.context) && s.canAdvanceSimulation(s.context)) s.action(s.context,HoleAction::drawCharacterPass1);
    s.action(s.context,HoleAction::drawSceneRoot);
    if (s.config.ghostShadows()) s.action(s.context,HoleAction::drawGroundShadows);
    if (!s.isEditing(s.context) && s.config.ghostShadows() && s.canAdvanceSimulation(s.context))
        s.action(s.context,HoleAction::drawCharacterPass2);
    s.action(s.context,HoleAction::drawSpatialDebug); s.action(s.context,HoleAction::advancePowerAnimations);
    s.action(s.context,HoleAction::drawLensEffects); s.action(s.context,HoleAction::drawCameraMarker);
    s.action(s.context,HoleAction::serviceNetwork);
    if (!s.isEditing(s.context) && s.config.ghostShadows() && s.canAdvanceSimulation(s.context)) {
        s.render.clearFlags(s.render.context,1); s.action(s.context,HoleAction::drawCharacterOcclusion);
    }
}
void Hole::drawHud(HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) return;
    if (!s.isEditing(s.context)) s.action(s.context,HoleAction::drawPowerHud);
    s.action(s.context,HoleAction::drawPlayersOverlay); s.action(s.context,HoleAction::drawPowerEditorPosition);
    s.action(s.context,HoleAction::drawEffects); s.action(s.context,HoleAction::drawSelectedSpatialDebug);
}
}
