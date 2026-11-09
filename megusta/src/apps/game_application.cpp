#include "game_application.h"
#include "game/driver_initialize.h"
#include "game/application_input.h"
#ifdef MEGUSTA_SDL
#include "platform/sdl/startup_host.h"
#else
#include "game/win32_startup_host.h"
#endif
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
#include "dts/mesh.h"
#include "game/effect.h"
#include "game/hud.h"
#include "game/spatial_editor.h"
#include "game/spatial_object.h"
#include "game/power_up.h"
#include "engine/multi_texture.h"
#include <memory>
#include "game/hole_render_backend.h"
#ifdef __EMSCRIPTEN__
#include "platform/browser.h"
#elif defined(MEGUSTA_SDL)
#include "platform/sdl/sdl_runtime.h"
#else
#include "platform/win32.h"
#endif
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
    inline static const char* const wireModeNames[7]={"grid","object","object face","collidable objects","all objects","flagged objects","grid"};
#ifdef __EMSCRIPTEN__
    platform::BrowserRuntime platform;
#elif defined(MEGUSTA_SDL)
    platform::SdlRuntime platform;
#else
    platform::Win32Runtime platform;
#endif
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
    SpatialDebugServices spatialDebugServices{gl,this,drawSpatialObject,drawSelectedSpatialObjectInfo};
    int spatialEditorMoveStep=1;
    Putter putter;
    PutterTuning putterTuning{6.5f,0.25f,12}; // 00502ca8/00502cac/00502cb0 from canonical Ghidra memory.
    float remotePowerThreshold=0.0f; // 0052bfc8; initial .data word.
    PuttGuide guide;
    PutterResourceServices putterResources{guide,nullptr,nullptr,this,0,0,nullptr,nullptr,sounds};
    int aimGuideLength=12; // Original data word 00502cb0, canonical Ghidra export.
    Random random{},fluffRandom{};
    SceneObjectCollection objects{};
    WireDebugServices wireServices{objects,sceneGrid,gameInput,false,false,wireModeNames,
        wireAllCallback,wireTriangleCallback,wireCandidatesCallback};
    PowerUps powers;
    guiSystem gui;
    GuiWidgetResources widgets;
    GuiAtlasRegistry atlases{resources.mounts()};
    GuiRenderer renderer{gl,atlases,widgets};
    GuiRuntime guiState{gui,gameInput};
    GuiShapeServices guiShapes{};
    bool renderLow=false,renderHigh=true;
    GuiDebugState guiDebug{false,0,renderLow,renderHigh};
    std::uint32_t softwareCursorTexture=0;
    int editorMode=0;
    iRoomClient* client=nullptr;
    int consoleColumns=80,consoleRows=20;
    bool consoleScaled=false;
    ConsoleServices consoleServices{config,client,gl,driverViewport.displayWidth,driverViewport.displayHeight,
        consoleColumns,consoleRows,consoleScaled,platform::gameMilliseconds,platform::gameQuery};
    PlayerServices playerServices{this,fatal,nullptr,nullptr,nullptr,config,random};
    // The original initializer thunk at 00457760 jumps to 00457770, which
    // stores 0, 0, and -the float at 004eafb0 (0.426666677f). The data bytes
    // start at zero; the initializer's runtime reachability is still unclear.
    float acceleration[3]={0.0f,0.0f,-0.426666677f};
    GameplayHost gameplayHost{playerServices,this,prepareDraw,restoreHeading,strikeSound,ballSound,objectSound,
        // Ball_resolveSceneImpact_impl reads these as compact digit opcodes,
        // not descriptive text. Values are the strings at 004f3788,
        // 00504368 and 0050436c respectively.
        milliseconds,networkEnd,acceleration,"0","321","123"};
    GameplayRuntime gameplay{gamePlayers,gameBalls,gameTasks,objects,sceneGrid,ballCollision,ballCollisionRandom,gameplayHost};
    PuttMap map;
    PuttMapResourceIO mapIO{resources.mounts()};
    PuttMapRuntimeHost mapHost{mapIO.services(),this,ballPosition,fatal};
    PuttMapRuntime mapRuntime{map,objects,config,random,mapHost};
    bindings::GameplayBindingServices gameplayBindings{playerServices,gameplay.rest,gameplay.gameplay,
        objects,map,gl,editorMode,powers.lastUsedPowerUpId,&mapRuntime};
    HoleRenderServices rendering=makeOpenGLHoleRenderServices(gl);
    BallsRenderState ballRendering{objects,sceneGrid,ballCollision,gamePlayers,gl,camera.viewMatrix[0]};
    SceneObjectAnimationServices animation{};
    CameraFrameServices cameraFrame{cameraState,gamePlayers,gameBalls,putter,gameSpatialObjects,objects,sceneGrid,gl,
        gameInput,cameraServices,animation,gameHoleClock.totalFixedTickCount,driverViewport.displayWidth,
        driverViewport.displayHeight,cameraCollisionFilter,gui.hasTabElements,false,
        platform::gameWarpCursor,[](const char* name,int loop){active->sounds.play(name,loop);}};
    PutterTurnRuntime putterTurnRuntime{};
    Effects effects{};
    HoleServices holeServices{gameHoleClock,gameplay,mapRuntime,gameDriver.activeScript,gameInput,gameInputState,
        putter.smoothedAim,guide,aimGuideLength,putterTuning,remotePowerThreshold,gui.hasTabElements,
        camera,cameraState,cameraServices,cameraFrame,softwareCursorTexture,ballRendering,gameBalls,putter,
        putterResources,driverViewport.displayWidth,driverViewport.displayHeight,effects,powers,
        sounds,gameSpatialObjects,spatialDebugServices,wire,wireServices,spatialEditor,editorMode,spatialEditorMoveStep,client,
        gameSceneRoots,animation,this,ambient,config,rendering};
    DriverAutorunServices autorun{config,gameHole,gamePlayers,gameInputState,this,milliseconds};
    DriverScriptServices scripts{holeServices,gui,client,sounds,this,fatal,driverAutorun,autorun," \t"};
    ApplicationInput input{platform,gameConsole,gui,gameInputState,driverViewport,gameHole,gamePlayers,config,scripts};
    DriverServices driver{gameHole,gameHoleClock,holeServices,driverViewport,scripts,powers.slowMotion,
        powers.slowMotionTickDivider,gui,guiState,guiDebug,gameConsole,client,sounds,camera,this,
        shellOpen,background,softwareCursor,cursorEnabled,tabs,coordinate,cameraCollision,milliseconds,sleep,display,
        driverDispatchJoystick};
    platform::DriverPlatformBinding callbacks{platform,gameDriver,input.services,driver};
    SceneLoadState sceneLoading{startup.sceneLoadMethod,startup.preferences.textureLimitIndex};
    PowerUpsRuntime powerState{powers,random,gamePlayers,gameBalls,objects,gameSpatialObjects,sceneGrid,ballCollision,
        gameplay.tasks,client,playerServices,gameInput,gl,resources.mounts(),camera.viewMatrix[0],gameDriver.activeScript,
        gameHoleClock.totalFixedTickCount};
    GameShapeRuntime shapes{gameShapeSequenceNames,camera,cameraState,gameBalls,putter,gamePlayers,objects,nullptr};
    bindings::GameVariables variables;
    EffectRuntime effectState{effects,config,gl,gameInput.mouse.x,gameInput.mouse.y,
        driverViewport.displayHeight,camera.viewMatrix[0],0,0,0,0};
    std::unique_ptr<PutterAnimationServices> putterBindingAnimation;
    std::unique_ptr<PutterStrikeServices> putterBindingStrike;
    std::unique_ptr<bindings::WorldBindingServices> worldBindings;
    std::uint32_t puttOverFrame=0,frameGate=0,lastAimFrame=0,aimInterval=0;
    std::unique_ptr<RoomClientRuntime> network;
    DriverInitialization initialization{config,startup,gameConsole,gameHoleClock,gameBalls,putter,guide,gameInputState,
        gamePlayers,random,fluffRandom,guiFonts.primary,guiFonts.console,guiFonts.secondary,widgets,resources,sounds,
        gui,guiState,sceneLoading,powerState,shapes,softwareCursorTexture,{},variables,client,callbacks,this,installExit,selectLoader};
#ifdef MEGUSTA_SDL
    SdlStartupHostContext startupContext{platform,driverAutorun,autorun,cameraState,startup.preferences.textureDepth,
        startup.preferences.textureLimitIndex,startup.preferences.vsyncMode};
    StartupServices host=makeSdlStartupServices(startupContext);
#else
    Win32StartupHostContext startupContext{platform,driverAutorun,autorun,cameraState,startup.preferences.textureDepth,
        startup.preferences.textureLimitIndex,startup.preferences.vsyncMode};
    StartupServices host=makeWin32StartupServices(startupContext);
#endif
    GameEntryServices entry;
    Application(int argc,char** argv):entry{resources,config,driverViewport,platform,startup,initialization,host,argc,argv} {
#ifdef __EMSCRIPTEN__
        // WebGL has no display-list backend. Keep the recovered immediate
        // rendering branches active for DTS meshes and their skin updates.
        dts::dtsDisplayListsEnabled=false;
        dts::dtsSkinDisplayListsEnabled=false;
#endif
        configurePutterResources();
        guiState.scriptServices=&scripts;
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
            nativeShapeAnimationServices(),sounds,gameHoleClock.totalFixedTickCount,false,gamePlayerVoiceNames});
        putterTurnRuntime={&putter,putterBindingAnimation.get(),&camera,&cameraFrame,&gameInput,&client,
            &gameHoleClock.totalFixedTickCount,&gameplay.tasks.scheduledTasks.tickCount,&editorMode};
        playerServices.putterTurn=&putterTurnRuntime;
        putterBindingStrike.reset(new PutterStrikeServices{gamePlayers.playerStorage[0],gameBalls,
            gameplay.gameplay,client,camera,cameraState});
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
        playerServices.powerUps=&powers; playerServices.powerUpDirty=&powerState.powerUpDirty; playerServices.client=&client;
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
        playerServices.acknowledgeFrameGate=[](void* p){return app(p).client && app(p).client->acknowledgeFrameGate();};
        playerServices.sendPickupPowerUp=[](void* p,std::uint32_t id){auto* c=app(p).client;return !c || c->sendPickupPowerUp(id);};
        playerServices.sendActivatePowerUp=[](void* p){if(auto* c=app(p).client)c->sendActivatePowerUp();};
        playerServices.playButtonSound=[](void* p){app(p).sounds.play("button",0);};
        playerServices.canProgressActivePutt=[](void* p){auto* c=app(p).client;return !c || c->canProgressActivePutt();};
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
    static void drawSpatialObject(void* p,SpatialObject& object) {
        auto& a=app(p);const float* view=a.camera.viewMatrix[0];const int filter=a.cameraFrame.typeFilter;
        if(auto* v=dynamic_cast<SpatialTarget*>(&object))v->drawDebug(a.gl,view,filter);
        else if(auto* v=dynamic_cast<SpatialRegion*>(&object))v->drawDebug(a.gl,view,filter,gameSpatialObjects);
        else if(auto* v=dynamic_cast<SpatialZone*>(&object))v->drawDebug(a.gl,view,filter);
        else if(auto* v=dynamic_cast<SpatialCamera*>(&object))v->drawDebug(a.gl,view,filter);
        else if(auto* v=dynamic_cast<SpatialRepel*>(&object)){const float z=gameBalls.currentBall?gameBalls.currentBall->position[2]:0.0f;v->drawDebug(a.gl,view,filter,gameSpatialObjects,z);}
        else if(auto* v=dynamic_cast<PowerUp*>(&object))v->drawDebug(view);
    }
    static void drawSelectedSpatialObjectInfo(void* p,SpatialObject& object) {
        auto& a=app(p);if(auto* power=dynamic_cast<PowerUp*>(&object))power->drawEditorInfo(a.powerState,editorFontRenderer);
    }
    void configurePutterResources() {
        PutterResourceServices& s=putterResources;
        s.loadAtlas=[](void* p,const char* path,bool external,int mip,int wrap,int min,int mag)->void*{
            auto& a=app(p);return pix::loadMultiTexture(a.resources.mounts(),path,external,mip,a.gl.tokens[wrap],a.gl.tokens[min],a.gl.tokens[mag]);};
        s.atlasTextureCount=[](void*,void* atlas){return static_cast<int>(static_cast<pix::MultiTextureList*>(atlas)->textureCount);};
        // Effect construction and configuration are direct game calls;
        // only archive-backed atlas loading crosses the resource adapter.
        s.fatal=fatal;
    }
    static void driverDispatchJoystick(void* p) { app(p).platform.dispatchJoystick(); }
};
}
void runGameApplication(void* instance,int argc,char** argv) {
    Application application(argc,argv);
#ifdef MEGUSTA_SDL
    (void)instance;
    gameRuntimeStartupBridge(argc,argv);
#else
    engineRuntimeWinMainBootstrap(instance);
#endif
}
}
