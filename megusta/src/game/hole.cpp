#include "hole.h"
#include "putt_map_runtime.h"
#include "room_client.h"
#include "balls.h"
#include "camera.h"
#include "putter.h"
#include "powerups.h"
#include "effect.h"
#include "sounds.h"
#include "spatial_object.h"
#include "scene_runtime.h"
#include "native_game_helpers.h"
#include "spatial_editor.h"
#include "hud.h"
#include "gui_shell.h"
#include "putt_guide.h"
#include "game_shape_adapter.h"
#include "shape_sequence_names.h"
#include "../engine/multi_texture.h"
#include "../engine/string_services.h"
#include <cstdlib>
namespace megusta {
namespace {
struct PutterControlFrame {
    const ShapeAnimationServices& shape;
    PutterAnimationServices animation;
    PutterStrikeServices strike;
    PutterHumanInputServices human;
    PutterAutomatedServices automated;
    PutterControlServices control;
    PutterControlFrame(HoleServices& s,Player& player)
        :shape(nativeShapeAnimationServices()),
         animation{&player,s.runtime.host.playerServices.random,shape,s.sounds,s.clock.totalFixedTickCount,
             s.editorMode!=0,gamePlayerVoiceNames},
         strike{player,s.balls,s.runtime.gameplay,s.client,s.camera,s.cameraRuntime},
         human{s.runtime.players,s.balls,s.runtime.host.playerServices.random,shape,animation,s.guide,
             s.runtime.objects,s.runtime.grid,s.runtime.collision,s.clock.totalFixedTickCount,
             s.input.buttons,s.input.previousButtons,s.input.mouse.x,s.input.mouse.y,s.aimGuideLength,
             s.camera.headingDegrees,s.remotePowerThreshold,s.input.joystickAxes[3],s.client,
             s.camera,s.cameraRuntime,s.sounds},
         automated{s.runtime.players,s.balls,s.puttMap,s.runtime.host.playerServices,
             s.runtime.host.playerServices.random,shape,animation,s.clock.totalFixedTickCount,
             s.runtime.tasks.scheduledTasks.tickCount,s.input.buttons,s.camera.checkControlCam(),
             s.client&&s.client->inGame(),s.camera.headingDegrees,s.remotePowerThreshold,
             s.client,s.camera,s.cameraRuntime},
         control{player,s.balls,shape,animation,s.putterTuning,s.guide,human,automated,strike,
             s.runtime.players,s.runtime.host.playerServices,s.sounds,s.hasTabElements,s.input.modifiers,
             s.input.keyCode,s.clock.totalFixedTickCount,s.camera.headingDegrees} {}
};
}
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
    s.puttMap.release(); s.spatialObjects.saveAndDestroy(ownedInitString1,nullptr);
    s.runtime.objects.releaseSceneRecords(); s.camera.endHole(s.cameraRuntime,s.cursorTexture!=0,s.cameraServices);
    s.balls.releaseHoleModels(s.ballRendering); s.putter.releaseHoleResources();
    s.runtime.players.release(s.runtime.host.playerServices); s.sounds.releaseTransientPlayback();
    s.effects.release(); networkServiceEnd();
    networkServiceEnd(); s.runtime.grid.releaseSceneIndex();
    s.powers.releaseHole(); gameHole.transitionState=0;
    std::free(ownedInitString0); ownedInitString0=nullptr; std::free(ownedInitString1); ownedInitString1=nullptr;
}
void Hole::initialize(const char* first,const char* second,int argument,HoleFrameClock& clock,HoleServices& s) {
    if (hasActiveScene(s.runtime.objects)) return;
    std::free(ownedInitString0); ownedInitString0=nullptr; std::free(ownedInitString1); ownedInitString1=nullptr;
    ownedInitString0=strings::duplicate(first); ownedInitString1=strings::duplicate(second); initIntegerArgument=argument;
    s.inputState.initialize(); transitionState=1;
    s.sceneRoots.initializeSecondary(); s.runtime.objects.buildFromScene(s.sceneRoots.root,s.animation);
    s.runtime.grid.buildSceneIndex(s.runtime.objects); s.sounds.releaseTransientPlayback();
    s.balls.loadHoleModels(s.ballRendering);
    s.runtime.players.initializeHole(clock.totalFixedTickCount,s.runtime.balls,s.runtime.tasks,s.runtime.host.playerServices);
    s.putterResources.sceneRoot=s.sceneRoots.root;
    s.putterResources.secondarySceneRoot=s.sceneRoots.secondaryRoot;
    s.putterResources.tick=clock.totalFixedTickCount;
    s.putterResources.screenWidth=s.displayWidth;
    s.putter.loadHoleResources(s.putterResources); s.camera.initializeHole(s.cameraRuntime,s.cameraServices);
    s.effects.initializeLensTexture(); networkServiceEnd();
    s.wire.initialize(); s.powers.loadHoleModels();
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
    s.spatialObjects.loadMapFile(ownedInitString1,nullptr);
    s.puttMap.loadBinary(s.currentScriptName,clock.totalFixedTickCount);
    s.puttMap.loadScript(s.currentScriptName);
}
void Hole::update(HoleFrameClock& clock,HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) return;
    if (transitionState==1) {
        PlayerInputContext inputContext{s.input,teePositions,s.smoothedAim,s.runtime.balls,s.runtime.tasks,s.runtime.rest,s.runtime.host.playerServices};
        s.runtime.players.handleInput(inputContext);
    } else if (transitionState==2) {
        if (clock.transitionDurationTicks<=clock.totalFixedTickCount-clock.transitionStartTick) transitionState=3;
    } else if (transitionState==3) s.runtime.players.startNextHoleTask(s.runtime.tasks,s.runtime.host.playerServices);
    if (s.editorMode==0 && s.runtime.players.currentPlayer) {
        PutterControlFrame putterControl{s,*s.runtime.players.currentPlayer};
        s.putter.dispatchInput(putterControl.control);
    }
    s.wireServices.debugKeys=s.config.debugKeys();
    s.wireServices.networkInGame=s.client&&s.client->inGame();
    handleWireInput(s.wire,s.wireServices); s.powers.handleInput();
    s.camera.handleInput(s.cameraFrame);
    SpatialEditorServices editorServices{s.camera,s.cameraFrame,s.powers,s.editorMode,s.spatialEditorMoveStep,
        s.config.debugKeys(),s.client&&s.client->inGame(),s.currentScriptName,".py"};
    s.spatialEditor.processInput(editorServices);
}
void Hole::processFrame(HoleFrameClock& clock,HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) return;
    s.runtime.stepBalls();
    if (s.editorMode==0 && s.runtime.players.currentPlayer) {
        PutterControlFrame putterControl{s,*s.runtime.players.currentPlayer};
        s.putter.advanceControl(putterControl.control);
    }
    s.runtime.players.advanceTurnIfReady(clock.totalFixedTickCount,s.runtime.balls,s.runtime.host.playerServices);
    s.effects.updateAll(); networkServiceEnd();
    s.powers.processPickups(); s.camera.applyInputMotion(s.cameraRuntime);
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
    if (!hasActiveScene(s.runtime.objects)) { drawSceneRoot(); return; }
    s.balls.draw(s.ballRendering); s.camera.updateFollowTarget(s.cameraFrame);
    PutterAimServices aimServices{s.runtime.players.currentPlayer,s.balls,s.guide,s.runtime.objects,s.runtime.grid,
        s.runtime.collision,s.camera.headingDegrees,s.aimGuideLength,s.client};
    s.putter.drawAimPreview(aimServices);
    if (s.balls.currentBall) s.wire.collectBallCandidates(s.runtime.grid,s.runtime.objects,*s.balls.currentBall);
    s.render.clearColor(s.render.context,0.25f,0.25f,0.25f,1.0f); s.render.clearFlags(s.render.context,3);
    setupHoleHudRenderState(s.render);
    if (s.editorMode==0 && (!s.client || s.client->canAdvanceSimulation()) && s.runtime.players.currentPlayer) {
        PutterDrawServices characterDraw{*s.runtime.players.currentPlayer,s.runtime.grid,s.runtime.objects,s.camera.viewMatrix[0]};
        s.putter.drawCharacterPass1(characterDraw);
    }
    drawSceneRoot();
    if (s.config.ghostShadows()) s.balls.drawGroundShadows(s.ballRendering);
    if (s.editorMode==0 && s.config.ghostShadows() && (!s.client || s.client->canAdvanceSimulation()) && s.runtime.players.currentPlayer) {
        PutterDrawServices characterDraw{*s.runtime.players.currentPlayer,s.runtime.grid,s.runtime.objects,s.camera.viewMatrix[0]};
        s.putter.drawCharacterPass2(characterDraw);
    }
    s.spatialObjects.drawAllDebug(s.spatialDebugServices); s.powers.advanceAnimations();
    s.effects.drawLensPassAll(); s.camera.drawEditorTargetMarker(s.cameraFrame);
    networkServiceEnd();
    if (s.editorMode==0 && s.config.ghostShadows() && (!s.client || s.client->canAdvanceSimulation())) {
        s.render.clearFlags(s.render.context,1);
        if (s.runtime.players.currentPlayer) {
            PutterDrawServices characterDraw{*s.runtime.players.currentPlayer,s.runtime.grid,s.runtime.objects,s.camera.viewMatrix[0]};
            s.putter.drawCharacterOcclusionPass(characterDraw);
        }
    }
}
void Hole::drawHud(HoleServices& s) {
    if (!hasActiveScene(s.runtime.objects)) return;
    if (s.editorMode==0 && s.runtime.players.currentPlayer && s.putter.hudAtlas) {
        auto* atlas=static_cast<pix::MultiTextureList*>(s.putter.hudAtlas);
        const auto texture=[atlas](PutterHudTextureSlot slot) {
            return pix::getMultiTexture(atlas,static_cast<int>(slot));
        };
        const PutterHudTextures textures{texture(PutterHudTextureSlot::leftBar),
            texture(PutterHudTextureSlot::rightBar),texture(PutterHudTextureSlot::highPowerBar),
            texture(PutterHudTextureSlot::simpleNeedle),texture(PutterHudTextureSlot::powerNeedle)};
        s.putter.drawPowerHud(*s.runtime.players.currentPlayer,!s.client||s.client->isTurnStateReady(),
            s.client&&s.client->shouldWaitForRemoteTurn(),textures);
    }
    static const float foreground[3]={1.0f,1.0f,1.0f};
    PlayerOverlayServices overlay{editorFontRenderer,guiFonts.primary,s.camera,s.balls,s.displayWidth,s.displayHeight,
        s.clock.totalFixedTickCount,s.config.nameTags(),foreground};
    s.runtime.players.drawOverlay(overlay); s.powers.drawEditorBallPosition(editorFontRenderer,guiFonts.primary);
    s.effects.drawAll(); s.spatialObjects.drawSelectedDebug(s.spatialDebugServices);
}
}
