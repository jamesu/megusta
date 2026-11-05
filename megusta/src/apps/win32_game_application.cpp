#include "win32_game_application.h"
#include "game/driver_initialize.h"
#include "game/application_input.h"
#include "game/win32_startup_host.h"
#include "game/putt_map_runtime.h"
#include "game/putt_map_io.h"
#include "game/gui_render.h"
#include "game/gui_shell.h"
#include "game/gui_shape_previews.h"
#include "game/gui_tooltip.h"
#include "game/gui_widgets.h"
#include "game/native_game_helpers.h"
#include "game/random.h"
#include "game/shape_sequence_names.h"
#include "dts/graphics_state.h"
#include "game/effect.h"
#include "game/hud.h"
#include "game/spatial_editor.h"
#include "game/spatial_object.h"
#include "game/power_up.h"
#include "engine/multi_texture.h"
#include <memory>
#include "game/hole_render_backend.h"
#include "platform/win32.h"
#include "engine/gl_token_table.h"
#include "bindings/game_variables.h"
#include "bindings/gameplay.h"
#include "bindings/world.h"
#include "bindings/swig_runtime.h"
#include "bindings/network.h"
#include "audio/openal.h"
#include "engine/runtime_adapters.h"
#include "game/task.h"
#include "ssg.h"
#include "pu.h"
#include <stdexcept>
#include <string>
#include <cstdlib>
namespace megusta {
namespace {
[[noreturn]] void missing(const char* name) { throw std::logic_error(std::string("Recovered game action/service invariant failed: ")+name); }
struct Application {
    inline static Application* active=nullptr;
    platform::Win32Runtime platform;
    ResourceRuntime resources;
    StartupState startup;
    AudioCategoryGains gains{1,1,1,1,1};
    Sounds sounds{gains,platform::gameMilliseconds};
    AudioGainServices audioGains{gains,[](void* p){static_cast<Sounds*>(p)->refreshPlayingGains();},&sounds};
    GameConfigRuntime config{"/mnt/memcard/minigolf.cfg",audioGains};
    Win32GLTokenTable tokens;
    OpenGLAdapter gl{tokens};
    Camera camera{};
    CameraRuntime cameraState{};
    int cameraCollisionFilter=0;
    bindings::CameraBindingServices cameraBindings{cameraState,gameHoleClock.totalFixedTickCount,cameraCollisionFilter,
        [](const char* text){gameConsole.echo(text);}};
    CameraServices cameraServices{[](){throw std::runtime_error("Recovered camera fatal boundary");},
        [](float w,float h){ssgSetFOV(w,h);},
        [](const char* name,int group){active->sounds.preload(name,group);},platform::gameSetCursorMode};
    WireDebugState wire;
    SpatialEditor spatialEditor;
    int spatialEditorMoveStep=1;
    Putter putter;
    PutterTuning putterTuning{6.5f,0.25f,12}; // 00502ca8/00502cac/00502cb0 from canonical Ghidra memory.
    float remotePowerThreshold=0.0f; // 0052bfc8; initial .data word.
    PuttGuide guide;
    int aimGuideLength=12; // Original data word 00502cb0, canonical Ghidra export.
    Random random{},fluffRandom{};
    SceneObjectCollection objects{};
    PowerUps powers;
    guiSystem gui;
    GuiWidgetResources widgets;
    GuiAtlasRegistry atlases{resources.mounts()};
    GuiRenderer renderer{gl,atlases,widgets};
    GuiRuntime guiState{gui,gameInput};
    GuiShapeServices guiShapes{};
    GuiShellActions shellActions{};
    bool renderLow=false,renderHigh=true;
    GuiDebugState guiDebug{false,0,renderLow,renderHigh};
    std::uint32_t softwareCursorTexture=0;
    int editorMode=0;
    iRoomClient* client=nullptr;
    int consoleColumns=80,consoleRows=20;
    bool consoleScaled=false;
    ConsoleServices consoleServices{config,client,gl,driverViewport.displayWidth,driverViewport.displayHeight,
        consoleColumns,consoleRows,consoleScaled,platform::gameMilliseconds,platform::gameQuery};
    PlayerServices playerServices{this,fatal,nullptr,nullptr,nullptr,nullptr,nullptr,config,random};
    float acceleration[3]={0,0,-0.01f};
    GameplayHost gameplayHost{playerServices,this,prepareDraw,restoreHeading,strikeSound,ballSound,objectSound,
        milliseconds,networkEnd,acceleration,"overlap recovery","slow recovery","fast recovery"};
    GameplayRuntime gameplay{gamePlayers,gameBalls,gameTasks,objects,sceneGrid,ballCollision,ballCollisionRandom,gameplayHost};
    PuttMap map;
    PuttMapResourceIO mapIO{resources.mounts()};
    PuttMapRuntimeHost mapHost{mapIO.services(),this,ballPosition,fatal};
    PuttMapRuntime mapRuntime{map,objects,config,random,mapHost};
    bindings::GameplayBindingServices gameplayBindings{playerServices,gameplay.rest,gameplay.gameplay,
        objects,map,gl,editorMode,powers.lastUsedPowerUpId,&mapRuntime};
    HoleRenderServices rendering=makeOpenGLHoleRenderServices(gl);
    BallsRenderState ballRendering{objects,sceneGrid,ballCollision,gamePlayers,gl,camera.viewMatrix[0]};
    HoleServices holeServices{gameplay,mapRuntime,gameDriver.activeScript,gameInput,gameInputState,putter.smoothedAim,
        this,holeAction,ambient,editing,canAdvance,config,rendering};
    DriverAutorunServices autorun{config,gameHole,gamePlayers,gameInputState,this,milliseconds,networkEnd};
    DriverScriptServices scripts{holeServices,this,scriptAction,fatal,driverAutorun,autorun," \t",&sounds};
    ApplicationInput input{platform,gameConsole,gui,gameInputState,driverViewport,gameHole,gamePlayers,config,scripts};
    DriverServices driver{gameHole,gameHoleClock,holeServices,driverViewport,scripts,powers.slowMotion,
        powers.slowMotionTickDivider,this,driverAction,shellOpen,background,softwareCursor,cursorEnabled,tabs,
        coordinate,cameraCollision,milliseconds,sleep,display,&sounds,&camera};
    platform::DriverPlatformBinding callbacks{platform,gameDriver,input.services,driver};
    SceneLoadState sceneLoading{startup.sceneLoadMethod,startup.preferences.textureLimitIndex};
    PowerUpsRuntime powerState{powers,random,gamePlayers,gameBalls,objects,gameSpatialObjects,sceneGrid,ballCollision,
        gameplay.tasks,client,playerServices,gameInput,gl,resources.mounts(),camera.viewMatrix[0],gameDriver.activeScript,
        gameHoleClock.totalFixedTickCount};
    GameShapeRuntime shapes{gameShapeSequenceNames,camera,cameraState,gameBalls,putter,gamePlayers,objects,nullptr};
    bindings::GameVariables variables;
    Effects effects{};
    EffectRuntime effectState{effects,config,gl,gameInput.mouse.x,gameInput.mouse.y,
        driverViewport.displayHeight,camera.viewMatrix[0],0,0,0,0};
    SceneObjectAnimationServices animation{};
    std::unique_ptr<PutterAnimationServices> putterBindingAnimation;
    std::unique_ptr<PutterStrikeServices> putterBindingStrike;
    std::unique_ptr<bindings::WorldBindingServices> worldBindings;
    CameraFrameServices cameraFrame{cameraState,gamePlayers,gameBalls,putter,gameSpatialObjects,objects,sceneGrid,gl,
        gameInput,cameraServices,animation,gameHoleClock.totalFixedTickCount,driverViewport.displayWidth,
        driverViewport.displayHeight,cameraCollisionFilter,gui.hasTabElements,false,
        platform::gameWarpCursor,[](const char* name,int loop){active->sounds.play(name,loop);}};
    std::uint32_t puttOverFrame=0,frameGate=0,lastAimFrame=0,aimInterval=0;
    std::unique_ptr<RoomClientRuntime> network;
    DriverInitialization initialization{config,startup,gameConsole,gameHoleClock,gameBalls,putter,guide,gameInputState,
        gamePlayers,random,fluffRandom,guiFonts.primary,guiFonts.console,guiFonts.secondary,widgets,resources,sounds,
        gui,guiState,sceneLoading,powerState,shapes,softwareCursorTexture,{},variables,client,callbacks,this,installExit,selectLoader};
    Win32StartupHostContext startupContext{platform,driverAutorun,autorun,cameraState,startup.preferences.textureDepth,
        startup.preferences.textureLimitIndex,startup.preferences.vsyncMode};
    StartupServices host=makeWin32StartupServices(startupContext);
    GameEntryServices entry;
    Application(int argc,char** argv):entry{resources,config,driverViewport,platform,startup,initialization,host,argc,argv} {
        active=this;
        platform::bindGamePlatform(&platform);
        dts::bindDisplaySizeProvider([]() -> const int* {
            static int dimensions[2];
            dimensions[0]=driverViewport.displayWidth;
            dimensions[1]=driverViewport.displayHeight;
            return dimensions;
        });
        bindings::bindDriverServices(&scripts,platform::gameEnableJoystick);
        bindings::bindCameraServices(&cameraBindings);
        putterBindingAnimation.reset(new PutterAnimationServices{gamePlayers.currentPlayer,random,
            nativeShapeAnimationServices(),this,gameHoleClock.totalFixedTickCount,false,gamePlayerVoiceNames,
            [](void* p,const char* name,int delay){app(p).sounds.playVoice(name,static_cast<std::uint32_t>(delay));}});
        putterBindingStrike.reset(new PutterStrikeServices{gamePlayers.playerStorage[0],gameBalls,gameplay.gameplay,this,
            [](void* p){auto* c=app(p).client;return c&&c->shouldWaitForRemoteTurn();},
            [](void* p,Point3F& vector){auto& a=app(p);vector={a.putter.aimTarget[0],a.putter.aimTarget[1],a.putter.aimTarget[2]};},
            [](void* p,float power,const Point3F& vector){auto* c=app(p).client;if(c)c->sendPutt(power,vector);},
            [](void* p){auto& a=app(p);auto* player=gamePlayers.currentPlayer;if(player)a.camera.enterPostShotFollow(a.cameraState,*player,*player->getBall(gameBalls));}});
        worldBindings.reset(new bindings::WorldBindingServices{animation,cameraState,cameraServices,
            *putterBindingAnimation,*putterBindingStrike,putterTuning,camera.headingDegrees,scripts,gameDriver,
            gameHoleClock.totalFixedTickCount,cameraCollisionFilter,startup.demo,
            [](const char* text){gameConsole.echo(text);},
            [](){auto& a=*active;a.wire.applySceneCallbacks(a.objects,sceneGrid,
                wireAllCallback,wireTriangleCallback,wireCandidatesCallback);},
            platform::gameEnableJoystick,
            [](Camera& value){auto& a=*active;value.dump(a.cameraFrame);},
            [](Putter& value){value.dump();}});
        bindings::bindWorldServices(worldBindings.get());
        playerServices.networkInGame=[](void* p){auto* c=app(p).client;return c && c->inGame();};
        playerServices.isEditing=editing;
        playerServices.timeMilliseconds=milliseconds;
        playerServices.serviceNetworkEnd=networkEnd;
        playerServices.unloadSoundGroup=[](void* p,unsigned group){app(p).sounds.unloadGroup(group);};
        playerServices.voiceNames=gamePlayerVoiceNames;
        playerServices.preloadVoice=[](void* p,const char* model,const char* voice,unsigned group){app(p).sounds.preloadVoice((std::string(model)+voice).c_str(),group);};
        playerServices.inputDeviceEnabled=[](void* p){return app(p).platform.systemProperty(5);};
        playerServices.inputDeviceCount=[](void* p){return app(p).platform.query(9);};
        playerServices.preloadButtonSound=[](void* p,int group){app(p).sounds.preload("button",group);};
        playerServices.reportMissingEvent=[](void*,int line){throw std::runtime_error("Player event invariant failed at recovered line "+std::to_string(line));};
        playerServices.finishPutt=[](void* p){if(app(p).client)app(p).client->finishPutt();};
        playerServices.logSetCurrent=[](void*,const char* name){gameConsole.echo(name);};
        playerServices.beginHoleTransition=[](void*){gameHole.beginTransition(gameHoleClock);};
        playerServices.acknowledgeFrameGate=[](void* p){return app(p).client && app(p).client->acknowledgeFrameGate();};
        playerServices.setPowerUpDirty=[](void* p){app(p).powerState.powerUpDirty=1;};
        playerServices.sendPickupPowerUp=[](void* p,std::uint32_t id){auto* c=app(p).client;return !c || c->sendPickupPowerUp(id);};
        playerServices.sendActivatePowerUp=[](void* p){if(auto* c=app(p).client)c->sendActivatePowerUp();};
        playerServices.playButtonSound=[](void* p){app(p).sounds.play("button",0);};
        playerServices.restartCameraFollow=[](void* p){auto& a=app(p);a.camera.restartFollow(a.cameraState);};
        playerServices.isPutterSettled=[](void* p){return !gamePlayers.currentPlayer ||
            app(p).putter.isTurnSettled(*gamePlayers.currentPlayer,gameBalls);};
        playerServices.canProgressActivePutt=[](void* p){auto* c=app(p).client;return !c || c->canProgressActivePutt();};
        playerServices.resetPowerUpTurnState=[](void* p){auto& a=app(p);
            a.powers.slowMotion=false; a.powers.lastUsedPowerUpId=0;
            a.powerState.powerUpDirty=1;
        };
        playerServices.beginPutterTurn=[](void* p){
            auto& a=app(p); Player& player=*gamePlayers.currentPlayer;
            const auto& shape=nativeShapeAnimationServices();
            PutterAnimationServices animation{&player,a.random,shape,&a,gameHoleClock.totalFixedTickCount,
                a.editorMode!=0,gamePlayerVoiceNames,
                [](void* context,const char* name,int delay){app(context).sounds.playVoice(name,static_cast<std::uint32_t>(delay));}};
            a.putter.beginTurn(player,a.random,gameHoleClock.totalFixedTickCount,
                a.gameplay.tasks.scheduledTasks.tickCount,animation,&a,
                [](void* context){auto& a=app(context);auto* current=gamePlayers.currentPlayer;
                    if(current)app(context).putter.captureInputAnchor(a.client&&a.client->shouldWaitForRemoteTurn(),
                        current->isHuman,gameInput.mouse.x,gameInput.mouse.y);},
                [](void* context){auto& a=app(context);a.camera.resetFollowState(a.cameraFrame);});
        };
        playerServices.forceDeadPlayerPickup=[](void* p){if(auto* c=app(p).client)c->forceDeadPlayerPickup();};
        bindings::bindGameplayServices(&gameplayBindings);
        bindEffectRuntime(effectState);
        bindBallsRenderState(ballRendering);
        animation.context=this;
        animation.findByName=[](void* p,const char* name){return app(p).objects.findByName(name);};
        animation.forgetContactObject=[](void*,int index){gameBalls.forgetRemovedContactObject(index);};
        animation.triggerContactTask=[](void* p,int index){auto& a=app(p);gameBalls.triggerObjectContactTask(index,a.objects,sceneGrid,ballCollision,a.gameplay.tasks);};
        bindings::bindSceneAnimationServices(&animation);
        bindConsoleServices(consoleServices);
        initialization.bindRoomClient=[](void* p,iRoomClient& client){
            auto& a=app(p);
            a.network.reset(new RoomClientRuntime{client,gameBalls,a.putter,a.effects,a.gameplay.gameplay,
                a.playerServices,a.scripts,a.config,a.objects,a.animation,a.random,
                a.puttOverFrame,a.frameGate,a.lastAimFrame,a.aimInterval});
            a.network->executePython=Driver::executePython;
            a.network->timeMilliseconds=platform::gameMilliseconds;
            bindRoomClientRuntime(*a.network);
        };
        bindEditingState(&editorMode);
        gameTasks.initializeHead(); putter.initialize(gameHoleClock.totalFixedTickCount);
        guiState.context=this;
        guiState.executePython=[](void*,const char* command){Driver::executePython(command);};
        guiState.milliseconds=milliseconds;
        guiState.fixedTick=[](void*){return gameHoleClock.totalFixedTickCount;};
        guiState.mouse=[](void*,int b,int s,int x,int y){puMouse(b,s,x,y);};
        guiState.soundPlaying=[](void* p,const char* n,int key){return app(p).sounds.isPlaying(n,key);};
        guiState.playSound=[](void* p,const char* n,int loop){app(p).sounds.play(n,loop);};
        guiState.warpCursor=[](void* p,int x,int y){driverViewport.unmapMouse(x,y);app(p).platform.warpCursor(x,y);};
        guiState.enableCursorMotion=[](void* p){app(p).platform.setCursorMode(platform::CursorMode::Released);};
        guiState.renderer=&renderer;
        guiState.loadPng=[](void* p,std::uint32_t& texture,const char* path,int& width,int& height){
            texture=pix::bindPngTexture(app(p).resources.mounts(),path,0,GL_CLAMP,GL_NEAREST,GL_NEAREST,&width,&height);
            return texture!=0;
        };
        guiState.loadScene=[](void*,const char* name,bool textures){return loadSceneModel(name,textures);};
        bindNativeGuiShapeServices(guiShapes); guiState.shapes=&guiShapes;
        shellActions.context=this;
        shellActions.configureAutorun=[](void* p,int menu,int game,int start){auto& a=app(p);driverAutorun.configure(menu,game,start!=0,a.autorun);};
        guiState.shellActions=&shellActions;
        bindGameEntryServices(entry);
    }
    static Application& app(void* p) { return *static_cast<Application*>(p); }
    static void fatal(void*) { throw std::runtime_error("Recovered game fatal-error boundary"); }
    static std::uint32_t milliseconds(void* p) { return app(p).platform.milliseconds(); }
    static void sleep(void* p,std::uint32_t value) { app(p).platform.sleepMicroseconds(value); }
    static void networkEnd(void*) { networkServiceEnd(); }
    static void updateAudioListener(void* p) {
        auto& a=app(p); const bool active=gameHole.hasActiveScene(a.objects);
        const auto& eye=a.camera.controlEye; const auto& look=a.camera.controlLookAt;
        const float position[3]={eye.x,eye.y,eye.z},target[3]={look.x,look.y,look.z};
        a.sounds.updateListener(active,position,target);
    }
    static const float* ballPosition(void*) { if (!gameBalls.currentBall) missing("current ball"); return gameBalls.currentBall->position; }
    static void prepareDraw(void* p) { auto& a=app(p);gameDriver.setup2dProjection(driverViewport,a.rendering); }
    static void restoreHeading(void* p) { Camera::restorePlayerHeading(app(p).cameraState,gamePlayers.currentPlayer); }
    static void strikeSound(void* p,const float* position) {
        // Ball_strike_impl @ 00445ec0 plays data string 004f3fd4 ("putt") in 3D.
        app(p).sounds.play3d("putt",position[0],position[1],position[2],0,0);
    }
    static void ballSound(void* p,const float* position) {
        // Ball_resolveContactResponse_impl @ 00446c90 plays 004f3fcc ("hitball") for ball hits.
        app(p).sounds.play3d("hitball",position[0],position[1],position[2],0,0);
    }
    static void objectSound(void* p,SceneObjectRecord& object,const float* position,const float* velocity) { object.playImpactSound(position,velocity,app(p).sounds); }
    static void ambient(void*,float r,float g,float b) { const GLfloat value[]={r,g,b,1};glLightModelfv(GL_LIGHT_MODEL_AMBIENT,value); }
    static bool editing(void* p) { return app(p).editorMode!=0; }
    static bool canAdvance(void* p) { auto* c=app(p).client;return !c || c->canAdvanceSimulation(); }
    static bool shellOpen(void*) { return isGuiShellOpen(); }
    // Driver_frameCallback_impl @ 00401140 enters the background pass when
    // either the single texture or the multi-texture background list exists.
    static bool background(void* p) {
        const auto& gui=app(p).gui;
        return gui.backgroundTexture!=0 || gui.backgroundTextures!=nullptr;
    }
    static bool softwareCursor(void* p) {
        // Driver_frameCallback_impl gates cursor visibility on the loaded
        // texture handle (00525a14), not on the preferences that requested it.
        return app(p).softwareCursorTexture!=0;
    }
    static bool cursorEnabled(void* p) { return !app(p).platform.state.cursorCaptured; }
    static bool tabs(void* p) { return app(p).gui.hasTabElements; }
    static bool coordinate(void* p) { auto* c=app(p).client; return !c || c->coordinateTurnReadiness(); }
    static bool cameraCollision(void*) { return Camera::collisionEnabled(); }
    static void display(void*,int low,int high) { puDisplay(low,high); }
    // Driver_initialize_impl @ 00401e40 saves the prior engine exit target and
    // replaces it with the cleanup body at 00401d80.
    static engine_runtime::ExitHandler installExit(void*) {
        return engine_runtime::setExitHandler(exitCleanup);
    }
    static void exitCleanup(int status) {
        Application& a=*active;
        a.sounds.stopNonStreaming();
        a.config.save(a.resources.mounts());
        gameConsole.releaseFontRenderer();
        a.config.clear();
        gameHole.releaseResources(a.holeServices);
        gameTasks.clear(a.gameplay.tasks);
        a.gui.clearScreen();
        closeGuiShell();
        if (gameSceneRoots.root) gameSceneRoots.root->removeAllKids();
        ssgLoaderOptions::releaseTexturePath();
        ssgLoaderOptions::releaseModelPath();
        ssgShutdown();
        puShutdown();
        delete guiFonts.primary; guiFonts.primary=nullptr;
        delete guiFonts.console; guiFonts.console=nullptr;
        delete guiFonts.secondary; guiFonts.secondary=nullptr;
        networkServiceEnd();
        if (gameDriver.previousExitHandler) gameDriver.previousExitHandler(status);
        std::exit(status);
    }
    static void selectLoader(void* p) { bindSceneLoading(app(p).sceneLoading); }
    PutterDrawServices playerDrawServices(Player& player) {
        return {player,sceneGrid,objects,this,
            [](void* p,Player& player){player.shapeAdapter.drawPreview(app(p).camera.viewMatrix[0],player.characterTransform[0],player.leftHanded);},
            [](void* p,Player& player){player.shapeAdapter.drawShadow(app(p).camera.viewMatrix[0],player.characterTransform[0],player.leftHanded);},
            [](void* p,Player& player,bool mirrored,float alpha){player.shapeAdapter.drawWithAlpha(app(p).camera.viewMatrix[0],player.characterTransform[0],mirrored,alpha);},networkEnd};
    }
    static int wireAllCallback(ssgEntity* entity) {
        if(entity && entity->isAKindOf(ssgTypeLeaf())) { sgVec4 color={0,0,0,0};static_cast<ssgLeaf*>(entity)->drawHighlight(color); }
        return 1;
    }
    static int wireTriangleCallback(ssgEntity* entity) {
        auto* leaf=entity&&entity->isAKindOf(ssgTypeLeaf())?static_cast<ssgLeaf*>(entity):nullptr;
        if(leaf && active->wire.selectedObjectIndex>0 && active->wire.selectedObjectIndex<=active->objects.objectCount) {
            const auto& object=active->objects.objects[active->wire.selectedObjectIndex];
            for(int i=0;i<object.triangleCount;++i) {
                const auto& triangle=object.triangles[i];
                if(triangle.leaf==leaf && triangle.leafTriangleIndex==active->wire.selectedTriangleIndex) {
                    sgVec4 color={0,0,0,0};leaf->drawHighlight(color,active->wire.selectedTriangleIndex);break;
                }
            }
        }
        return 1;
    }
    static int wireCandidatesCallback(ssgEntity* entity) {
        auto* leaf=entity&&entity->isAKindOf(ssgTypeLeaf())?static_cast<ssgLeaf*>(entity):nullptr;
        if(leaf) for(int i=0;i<sceneGrid.candidateCount;++i) {
            const auto& triangle=active->objects.triangleStorage[sceneGrid.candidateIds[i]];
            if(triangle.leaf==leaf) { sgVec4 color={0,0,0,0};leaf->drawHighlight(color,triangle.leafTriangleIndex); }
        }
        return 1;
    }
    static void holeAction(void* p,HoleAction action) {
        auto& a=app(p);
        if(action==HoleAction::initializeSceneRoot){gameSceneRoots.initializeSecondary();return;}
        if(action==HoleAction::buildSceneObjects){a.objects.buildFromScene(gameSceneRoots.root,a.animation);return;}
        if(action==HoleAction::releaseTransientSounds){a.sounds.releaseTransientPlayback();return;}
        if(action==HoleAction::loadBallModels){gameBalls.loadHoleModels(a.ballRendering);return;}
        if(action==HoleAction::initializeCamera){a.camera.initializeHole(a.cameraState,a.cameraServices);return;}
        if(action==HoleAction::initializeWireDebug){a.wire.initialize();return;}
        if(action==HoleAction::releaseCamera){
            a.camera.endHole(a.cameraState,a.softwareCursorTexture!=0,a.cameraServices);return;
        }
        if(action==HoleAction::releaseBallModels){gameBalls.releaseHoleModels(a.ballRendering);return;}
        if(action==HoleAction::drawBalls){gameBalls.draw(a.ballRendering);return;}
        if(action==HoleAction::updateCameraFollow){a.camera.updateFollowTarget(a.cameraFrame);return;}
        if(action==HoleAction::drawAimPreview){
            PutterAimServices s{gamePlayers.currentPlayer,gameBalls,a.guide,a.objects,sceneGrid,ballCollision,p,
                a.camera.headingDegrees,a.aimGuideLength,networkEnd,
                [](void* p){return app(p).client && app(p).client->shouldWaitForRemoteTurn();},canAdvance,
                [](void* p,const Point3F& aim){if(app(p).client)app(p).client->sendAimVector(aim);}};
            a.putter.drawAimPreview(s);return;
        }
        if(action==HoleAction::collectWireCandidates){a.wire.collectBallCandidates(sceneGrid,a.objects,*gameBalls.currentBall);return;}
        if(action==HoleAction::drawCharacterPass1 || action==HoleAction::drawCharacterPass2 || action==HoleAction::drawCharacterOcclusion){
            if(!gamePlayers.currentPlayer)throw std::logic_error("Character rendering requires a current player");
            auto s=a.playerDrawServices(*gamePlayers.currentPlayer);
            if(action==HoleAction::drawCharacterPass1)a.putter.drawCharacterPass1(s);
            else if(action==HoleAction::drawCharacterPass2)a.putter.drawCharacterPass2(s);
            else a.putter.drawCharacterOcclusionPass(s);return;
        }
        if(action==HoleAction::drawGroundShadows){gameBalls.drawGroundShadows(a.ballRendering);return;}
        if(action==HoleAction::advancePowerAnimations){a.powers.advanceAnimations();return;}
        if(action==HoleAction::drawLensEffects){a.effects.drawLensPassAll();return;}
        if(action==HoleAction::drawEffects){a.effects.drawAll();return;}
        if(action==HoleAction::drawCameraMarker){a.camera.drawEditorTargetMarker(a.cameraFrame);return;}
        if(action==HoleAction::drawPowerEditorPosition){a.powers.drawEditorBallPosition(editorFontRenderer,guiFonts.primary);return;}
        if(action==HoleAction::drawPowerHud){
            if(!gamePlayers.currentPlayer || !a.putter.hudAtlas)return;
            auto* atlas=static_cast<pix::MultiTextureList*>(a.putter.hudAtlas);
            // The original PIX record is [count, textureNames...]. The recovered
            // x86 binds at offsets +0x10, +0x14, +0x18, +0x1c and +0x20 from
            // this record select texture IDs 3 through 7. The decompiler's
            // `putterHudAtlas[n].textureCount/textureNames[0]` notation is an
            // artifact of treating this flexible record as an array of structs.
            const auto texture=[](pix::MultiTextureList* list,PutterHudTextureSlot slot) {
                return pix::getMultiTexture(list,static_cast<int>(slot));
            };
            const PutterHudTextures textures{
                texture(atlas,PutterHudTextureSlot::leftBar),
                texture(atlas,PutterHudTextureSlot::rightBar),
                texture(atlas,PutterHudTextureSlot::highPowerBar),
                texture(atlas,PutterHudTextureSlot::simpleNeedle),
                texture(atlas,PutterHudTextureSlot::powerNeedle)};
            const bool ready=!a.client || a.client->isTurnStateReady();
            const bool waiting=a.client && a.client->shouldWaitForRemoteTurn();
            a.putter.drawPowerHud(*gamePlayers.currentPlayer,ready,waiting,textures);return;
        }
        if(action==HoleAction::drawPlayersOverlay){
            static const float foreground[3]={1.0f,1.0f,1.0f};
            PlayerOverlayServices s{editorFontRenderer,guiFonts.primary,a.camera,gameBalls,
                driverViewport.displayWidth,driverViewport.displayHeight,gameHoleClock.totalFixedTickCount,
                a.config.nameTags(),foreground};
            gamePlayers.drawOverlay(s);return;
        }
        if(action==HoleAction::drawSpatialDebug || action==HoleAction::drawSelectedSpatialDebug){
            SpatialDebugServices s{a.gl,&a,
                [](void* p,SpatialObject& object){
                    auto& a=app(p);const float* view=a.camera.viewMatrix[0];const int filter=a.cameraFrame.typeFilter;
                    if(auto* v=dynamic_cast<SpatialTarget*>(&object))v->drawDebug(a.gl,view,filter);
                    else if(auto* v=dynamic_cast<SpatialRegion*>(&object))v->drawDebug(a.gl,view,filter,gameSpatialObjects);
                    else if(auto* v=dynamic_cast<SpatialZone*>(&object))v->drawDebug(a.gl,view,filter);
                    else if(auto* v=dynamic_cast<SpatialCamera*>(&object))v->drawDebug(a.gl,view,filter);
                    else if(auto* v=dynamic_cast<SpatialRepel*>(&object)){
                        const float ballZ=gameBalls.currentBall?gameBalls.currentBall->position[2]:0.0f;
                        v->drawDebug(a.gl,view,filter,gameSpatialObjects,ballZ);
                    } else if(auto* v=dynamic_cast<PowerUp*>(&object))v->drawDebug(view);
                },
                [](void* p,SpatialObject& object){
                    auto& a=app(p);if(auto* power=dynamic_cast<PowerUp*>(&object))
                        power->drawEditorInfo(a.powerState,editorFontRenderer);
                }};
            if(action==HoleAction::drawSpatialDebug)gameSpatialObjects.drawAllDebug(s);
            else gameSpatialObjects.drawSelectedDebug(s);
            return;
        }
        if(action==HoleAction::putterInput || action==HoleAction::advancePutter){
            if(!gamePlayers.currentPlayer)return;
            Player& player=*gamePlayers.currentPlayer;const auto& shape=nativeShapeAnimationServices();
            PutterAnimationServices animation{&player,a.random,shape,&a,gameHoleClock.totalFixedTickCount,
                a.editorMode!=0,gamePlayerVoiceNames,
                [](void* p,const char* name,int delay){app(p).sounds.playVoice(name,static_cast<std::uint32_t>(delay));}};
            auto waitRemote=[](void* p){auto* c=app(p).client;return c&&c->shouldWaitForRemoteTurn();};
            auto stopCamera=[](void* p){auto& a=app(p);return a.camera.stopControlCam(a.cameraState);};
            auto stopCameraVoid=[](void* p){auto& a=app(p);a.camera.stopControlCam(a.cameraState);};
            PutterStrikeServices strike{player,gameBalls,a.gameplay.gameplay,&a,waitRemote,
                [](void* p,Point3F& vector){auto& a=app(p);vector={a.putter.aimTarget[0],a.putter.aimTarget[1],a.putter.aimTarget[2]};},
                [](void* p,float power,const Point3F& vector){auto* c=app(p).client;if(c)c->sendPutt(power,vector);},
                [](void* p){auto& a=app(p);auto* player=gamePlayers.currentPlayer;if(player)a.camera.enterPostShotFollow(a.cameraState,*player,*player->getBall(gameBalls));}};
            PutterHumanInputServices human{gamePlayers,gameBalls,a.random,shape,animation,a.guide,a.objects,sceneGrid,
                ballCollision,&a,gameHoleClock.totalFixedTickCount,gameInput.buttons,gameInput.previousButtons,
                gameInput.mouse.x,gameInput.mouse.y,a.aimGuideLength,a.camera.headingDegrees,a.remotePowerThreshold,
                gameInput.joystickAxes[3],waitRemote(&a),!a.client||a.client->isTurnStateReady(),stopCamera,
                [](void* p,const char* name,int loop){app(p).sounds.play(name,loop);}};
            PutterAutomatedServices automated{gamePlayers,gameBalls,a.mapRuntime,a.playerServices,a.random,shape,animation,
                &a,gameHoleClock.totalFixedTickCount,a.gameplay.tasks.scheduledTasks.tickCount,gameInput.buttons,
                a.camera.checkControlCam(),a.client&&a.client->inGame(),a.camera.headingDegrees,a.remotePowerThreshold,
                waitRemote,[](void* p){auto* c=app(p).client;return !c||c->isTurnStateReady();},stopCameraVoid,fatal};
            PutterControlServices control{player,gameBalls,shape,animation,a.putterTuning,a.guide,human,automated,strike,
                &a,gamePlayers.isActive(),a.gui.hasTabElements,waitRemote(&a),gameInput.modifiers,gameInput.keyCode,
                gameHoleClock.totalFixedTickCount,a.camera.headingDegrees,
                [](void* p){auto& a=app(p);showPutterPowerCurve(a.putterTuning,&a,
                    [](void* context,const char* text,int duration){auto& a=app(context);gamePlayers.showMessage(text,static_cast<std::uint32_t>(duration),gameHoleClock.totalFixedTickCount);});},
                nullptr,nullptr,
                [](void* p){auto* effect=static_cast<Effect*>(app(p).putter.effect);if(effect)effect->advanceParticles();},
                [](void* p){auto* effect=static_cast<Effect*>(app(p).putter.effect);if(effect)effect->stop();},
                [](void* p,int count){auto* effect=static_cast<Effect*>(app(p).putter.effect);if(effect)effect->warmup(count);},
                [](void* p){auto* effect=static_cast<Effect*>(app(p).putter.effect);if(effect)effect->start();},
                [](void* p,const char* name,int loop){app(p).sounds.play(name,loop);},fatal};
            if(action==HoleAction::putterInput)a.putter.dispatchInput(control);else a.putter.advanceControl(control);
            return;
        }
        if(action==HoleAction::loadPutter){
            PutterResourceServices s{a.guide,gameSceneRoots.root,gameSceneRoots.secondaryRoot,p,
                gameHoleClock.totalFixedTickCount,driverViewport.displayWidth};
            s.loadAtlas=[](void* p,const char* path,bool external,int mip,int wrap,int min,int mag)->void*{
                auto& a=app(p);return pix::loadMultiTexture(a.resources.mounts(),path,external,mip,a.gl.tokens[wrap],a.gl.tokens[min],a.gl.tokens[mag]);};
            s.atlasTextureCount=[](void*,void* atlas){return static_cast<int>(static_cast<pix::MultiTextureList*>(atlas)->textureCount);};
            s.preloadSound=[](void* p,const char* name,int group){app(p).sounds.preload(name,group);};
            // Putter_loadHoleResources_impl allocates this emitter through
            // Effect_initializeUnregistered_impl, not Effect_ctor (which adds
            // it to Effects::effects). At 0042f089 the original caller pushes
            // 0x40400000 (3.0f) before calling that initializer. The putter
            // owns and deletes this unregistered HUD effect before
            // Effects_release_impl runs.
            s.createEffect=[](void*)->void*{return new Effect(3.0f);};
            s.setEffectTexture=[](void*,void* effect,const char* path){static_cast<Effect*>(effect)->setTexture(path);};
            s.setEffectPosition=[](void*,void* effect,float x,float y,float z,float angle){static_cast<Effect*>(effect)->setPosition(x,y,z,angle);};
            s.setEffectScalar=[](void*,void* effect,PutterEffectScalar field,float value){auto& e=*static_cast<Effect*>(effect);switch(field){
                case PutterEffectScalar::rate:e.setRate(value);break;case PutterEffectScalar::duration:e.setDuration(value);break;
                case PutterEffectScalar::scale:e.setScale(value);break;case PutterEffectScalar::friction:e.setFriction(value);break;
                case PutterEffectScalar::ramp:e.setRamp(value);break;}};
            s.setEffectVector=[](void*,void* effect,PutterEffectVector field,float x,float y,float z){auto& e=*static_cast<Effect*>(effect);switch(field){
                case PutterEffectVector::direction:e.setDirection(x,y,z);break;case PutterEffectVector::dispersion:e.setDispersion(x,y,z);break;
                case PutterEffectVector::gravity:e.setGravity(x,y,z);break;case PutterEffectVector::volume:e.setVolume(x,y,z);break;}};
            s.warmupEffect=[](void*,void* effect,int count){static_cast<Effect*>(effect)->warmup(count);};s.fatal=fatal;
            a.putter.loadHoleResources(s);return;
        }
        if(action==HoleAction::initializeEffects){a.effects.initializeLensTexture();return;}
        if(action==HoleAction::releaseEffects){a.effects.release();return;}
        if(action==HoleAction::updateEffects){a.effects.updateAll();return;}
        if(action==HoleAction::loadPowerUps){a.powers.loadHoleModels();return;}
        if(action==HoleAction::releasePowerUps){a.powers.releaseHole();return;}
        if(action==HoleAction::loadSpatialObjects){gameSpatialObjects.loadMapFile(gameHole.ownedInitString1,nullptr);return;}
        if (action==HoleAction::drawSceneRoot) { drawSceneRoot();return; }
        if (action==HoleAction::serviceNetwork) { networkServiceEnd();return; }
        if(action==HoleAction::releasePuttMap){a.mapRuntime.release();return;}
        if(action==HoleAction::saveSpatialObjects){gameSpatialObjects.saveAndDestroy(gameHole.ownedInitString1,nullptr);return;}
        if(action==HoleAction::releasePutter){a.putter.releaseHoleResources(&a,
            [](void* p){auto& a=app(p);pix::destroyMultiTexture(static_cast<pix::MultiTextureList*>(a.putter.hudAtlas));},
            [](void*,void* effect){static_cast<Effect*>(effect)->releaseTexture();delete static_cast<Effect*>(effect);});return;}
        if(action==HoleAction::loadPuttMapBinary){a.mapRuntime.loadBinary(gameDriver.activeScript,gameHoleClock.totalFixedTickCount);return;}
        if(action==HoleAction::loadPuttMapScript){a.mapRuntime.loadScript(gameDriver.activeScript);return;}
        if(action==HoleAction::powerUpInput){a.powers.handleInput();return;}
        if(action==HoleAction::processPickups){a.powers.processPickups();return;}
        if(action==HoleAction::cameraInput){a.camera.handleInput(a.cameraFrame);return;}
        if(action==HoleAction::applyCameraInput){a.camera.applyInputMotion(a.cameraState);return;}
        if(action==HoleAction::spatialEditorInput){
            SpatialEditorServices s{a.camera,a.cameraFrame,a.powers,a.editorMode,a.spatialEditorMoveStep,
                a.config.debugKeys(),a.client&&a.client->inGame(),gameDriver.activeScript,".py"};
            a.spatialEditor.processInput(s);return;
        }
        if(action==HoleAction::wireInput){
            static const char* const names[]={"grid","object","object face","collidable objects","all objects","flagged objects","grid"};
            WireDebugServices s{a.objects,sceneGrid,gameInput,a.config.debugKeys(),a.client&&a.client->inGame(),names,
                wireAllCallback,wireTriangleCallback,wireCandidatesCallback};
            handleWireInput(a.wire,s);return;
        }
        missing(("HoleAction "+std::to_string(static_cast<int>(action))).c_str());
    }
    static void scriptAction(void* p,DriverScriptAction action) {
        auto& a=app(p);
        switch(action) {
        case DriverScriptAction::stopNonStreamingSounds:a.sounds.stopNonStreaming();return;
        case DriverScriptAction::serviceNetwork:networkServiceEnd();return;
        case DriverScriptAction::unloadGuiSoundGroup:a.sounds.unloadGroup(2);return;
        case DriverScriptAction::clearGuiScreen:a.gui.clearScreen();return;
        case DriverScriptAction::clearNetworkUiCallbacks:if(a.client)a.client->clearUiCallbacks();return;
        case DriverScriptAction::cleanupWidgets:puCleanUpJunk();return;
        case DriverScriptAction::applyDefaultSceneState:ssgGetCurrentContext()->forceBasicState();return;
        case DriverScriptAction::clearSceneRoots:gameSceneRoots.clear();return;
        case DriverScriptAction::resetGameBindings:bindings::swig_init_gamec_bindings(true);return;
        case DriverScriptAction::resetNetworkBindings:bindings::swig_init_network_bindings(true);return;
        case DriverScriptAction::registerGameBindings:bindings::swig_init_gamec_bindings(false);return;
        case DriverScriptAction::registerNetworkBindings:bindings::swig_init_network_bindings(false);return;
        default:missing(("DriverScriptAction "+std::to_string(static_cast<int>(action))).c_str());
        }
    }
    static void driverAction(void* p,DriverAction action) {
        auto& a=app(p);
        switch(action) {
        case DriverAction::updateGuiFrameDelta:
            a.guiState.screenWidth=driverViewport.displayWidth;
            a.guiState.screenHeight=driverViewport.displayHeight;
            a.gui.updateFrameDelta();return;
        case DriverAction::hideCursor:puHideCursor();return;
        case DriverAction::showCursor:puShowCursor();return;
        case DriverAction::drawGuiBackground:a.gui.drawShellBackground();return;
        case DriverAction::updateAudioListener:updateAudioListener(p);return;
        case DriverAction::drawPerformanceOverlay:drawPerformanceOverlay(guiFonts.primary,gameHoleClock,
            driverViewport.displayWidth,driverViewport.displayHeight,a.client&&a.client->inGame());return;
        case DriverAction::serviceNetwork:networkServiceEnd();return;
        case DriverAction::drawGuiRoot:a.gui.drawRoot();return;
        case DriverAction::drawChatOverlay:if(a.client)drawGuiChatOverlay(*a.client,a.objects.sceneRoot!=nullptr,gameDriver.activeScript,a.guiDebug);return;
        case DriverAction::drawTooltip:drawGuiTooltip();return;
        case DriverAction::drawConsoleOverlay:gameConsole.drawOverlay();return;
        case DriverAction::flushScenePreviews:flushGuiScenePreview();return;
        case DriverAction::updateGuiInteraction:a.gui.updateInteraction(a.config.debugKeys(),a.client&&a.client->inGame(),a.guiDebug);return;
        case DriverAction::advanceZoom:a.gui.advanceZoomAnimations();return;
        case DriverAction::processTurnSync:if(a.client)a.client->processTurnSync();return;
        case DriverAction::pollDisconnect:if(a.client)a.client->pollDisconnectAndTurnState();return;
        case DriverAction::dispatchJoystick:a.platform.dispatchJoystick();return;
        default:missing(("DriverAction "+std::to_string(static_cast<int>(action))).c_str());
        }
    }
};
}
void runWin32GameApplication(void* instance,int argc,char** argv) {
    Application application(argc,argv);
    engineRuntimeWinMainBootstrap(instance);
}
}
