#include "driver_initialize.h"
#include "engine/opengl_adapter.h"
#include "python_resource_import.h"
#include "python/runtime_setup.h"
#include "bindings/game_variables.h"
#include "bindings/sound_bindings.h"
#include "bindings/config_tasks.h"
#include "gui_shell.h"
#include "native_game_helpers.h"
#include "common_sound_literals.h"
#include "startup_literals.h"
#include "gui_shape_previews.h"
#include "ground_shadow.h"
#include "hud.h"
#include "dts/mesh.h"
#include "dts/resource_bridge.h"
#include "engine/runtime_adapters.h"
#include "bindings/gameplay.h"
#include "ssg.h"
#include "fnt.h"
#include "pu.h"
#include <GL/glpng.h>
#include <new>
#include <stdexcept>
namespace megusta {
// 00401e40: order follows the complete saved body. SEH/compiler cleanup is
// delegated to the host C++ runtime; no invented packed font or client views.
void Driver::initialize(DriverInitialization& s) {
    if (&s.callbacks.driver!=this || !s.installExitHandler || !s.selectDefaultSceneLoader)
        throw std::logic_error("Driver initialization requires native service bindings");
    if (&s.guiRuntime.system!=&s.gui || &s.guiRuntime.input!=&s.input.frame)
        throw std::logic_error("GUI runtime must refer to the driver GUI and input objects");
    bindGuiRuntime(s.guiRuntime);
    // Compatibility: PLIB font I/O uses the same mounted streams as the game;
    // retain stdio as PLIB's standalone default, without extracting temp files.
    fntSetFileCallbacks(
        [](const char* path,const char* mode)->void* { return sharedIo->open(path,mode); },
        [](void* out,std::size_t size,std::size_t count,void* file)->std::size_t {
            return static_cast<ResourceFile*>(file)->read(out,static_cast<std::uint32_t>(size),static_cast<std::uint32_t>(count));
        },
        [](void* file)->int { return sharedIo->close(static_cast<ResourceFile*>(file)); });
    bindDemoState(&s.startup.demo);
    bindSceneLoading(s.sceneLoading);
    if (&s.powerUpsRuntime.players!=&s.players || &s.powerUpsRuntime.balls!=&s.balls ||
        &s.powerUpsRuntime.input!=&s.input.frame || &s.powerUpsRuntime.network!=&s.roomClient)
        throw std::logic_error("Powerup runtime must share the driver gameplay and input state");
    bindPowerUpsRuntime(s.powerUpsRuntime);
    if (&s.shapes.players!=&s.players || &s.shapes.balls!=&s.balls || &s.shapes.putter!=&s.putter || &s.shapes.objects!=&s.powerUpsRuntime.objects)
        throw std::logic_error("Shape runtime must share the driver game state");
    bindGameShapeRuntime(s.shapes);
    if (s.guiRuntime.shapes) bindNativeGuiShapeServices(*s.guiRuntime.shapes);
    dts::dtsRenderGL=&s.powerUpsRuntime.gl; groundShadowCache.gl=&s.powerUpsRuntime.gl;
    s.powerUpsRuntime.gl.loadMultitextureFunctions();
    bindGroundShadowScene(s.powerUpsRuntime.objects,s.powerUpsRuntime.grid);
    dts::bindIflResourceService(s.resources.services().open,s.resources.services().close);
    s.movieHost={s.callbacks.runtime.nativeWindowHandle(),s.callbacks.runtime.capabilities().graphics}; engine_movie::bindMovieHost(s.movieHost);
    auto& playerServices=s.callbacks.services.holeServices.runtime.host.playerServices;
    playerServices.releasePlayerShape=[](void*,Player& player){player.shapeAdapter.release();};
    playerServices.loadPlayerShape=[](void*,Player& player,unsigned mask,bool shadow){player.shapeAdapter.load(player.modelTag,mask,shadow);};
    playerServices.setPlayerSkin=[](void*,Player& player,int skin){player.shapeAdapter.setSkin(skin);};
    if (auto* bindings=bindings::gameplayServices()) bindings->scriptRuntime=&s.callbacks.services.holeServices.puttMap;
    s.gui.initialize();
    previousExitHandler=s.installExitHandler(s.context);
    int gamma=s.config.readInt("Prefs","Gamma",5);
    if (gamma>=10) gamma=10;
    else if (gamma<2) gamma=1;
    // 0.2 is double in the saved expression: do not round its product to float.
    pngSetViewingGamma(1.0/(static_cast<float>(gamma)*0.2));
    s.selectDefaultSceneLoader(s.context);
    s.startup.forcePix=false;
    s.startup.sceneLoadMethod=2;
    activeScript[0]=pendingScript[0]=0;
    fpsOverlayEnabled=false;
    frameLimitEnabled=true;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    ssgInit();
    ssgSetFOV(60.0f,0.0f);
    ssgSetNearFar(2.0f,2000.0f);
    ssgModelPath("data/shapes");
    ssgTexturePath("data/textures");
    // Saved engine tokens 0x50=GL_LINEAR, 0x54=GL_LINEAR_MIPMAP_LINEAR.
    s.primaryFont=new(std::nothrow) fntTexFont("data/fonts/casual30.txf",GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);
    s.consoleFont=new(std::nothrow) fntTexFont("data/fonts/console.txf",GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);
    s.secondaryFont=new(std::nothrow) fntTexFont("data/fonts/adela30.txf",GL_LINEAR,GL_LINEAR);
    guiFonts.primary=s.primaryFont; guiFonts.console=s.consoleFont; guiFonts.secondary=s.secondaryFont;
    s.guiRuntime.resolveFont=[](void*,int index) { return resolveGuiFont(index); };
    puInit();
#ifdef _WIN32
    puSetNativeWindow(static_cast<HWND>(s.callbacks.runtime.nativeWindowHandle()));
#endif
    puSetDefaultFonts(puFont(s.primaryFont,18.0f,0.0f),puFont(s.primaryFont,18.0f,0.0f));
    // 00460e40 writes 005074f0, subsequently copied into widget.style by
    // its constructor. The old recovered callback-role name is misleading.
    puSetDefaultStyle(PUSTYLE_SMALL_SHADED);
    puSetDefaultColourScheme(0.6f,0.6f,1.0f,1.0f);
    auto& runtime=s.callbacks.runtime;
    engine_runtime::bindLaunchWindow(runtime.nativeWindowHandle());
    s.softwareCursorTexture=0;
    if (s.config.readInt("Prefs","SoftwareMouseCursor",0)!=0 &&
        s.config.readInt("Prefs","FullScreen",0)!=0) {
        loadGuiPngTexture(s.resources.mounts(),s.softwareCursorTexture,"data/textures/gui/cursor_1.png");
        puSetCursorTexture(s.softwareCursorTexture,-1,3,32,32,false);
        runtime.setCursorMode(platform::CursorMode::Captured);
    }
    s.console.initialize("Minigolf Console",startup_literals::consolePrompt,s.consoleFont);
    s.console.echo("Minigolf Maniacs v1.0");
    s.console.setCommandHandler(Driver::executePython);
    s.resources.capture().setSink([](const char* text,void* console) {
        static_cast<Console*>(console)->echo(text);
    },&s.console);
    s.firstRandom.setSeed(runtime.milliseconds());
    s.secondRandom.setSeed(runtime.milliseconds());
    s.clock.resetFrameCounters(runtime.milliseconds());
    gameSceneRoots.initialize(); // Shared with Scene bindings and native scene drawing.
    s.sounds.preloadCommon(common_sound_literals::first,common_sound_literals::seventh);
    s.balls.initialize([](void* context) {
        return pix::bindPngTexture(*static_cast<ResourceMountTable*>(context),"DATA/TEXTURES/GOLFBALS.PNG",
            PNG_NOMIPMAPS,GL_CLAMP,GL_NEAREST,GL_NEAREST);
    },&s.resources.mounts());
    // The original seeds PuttGuide animation from totalFixedTickCount, not
    // wall-clock milliseconds. Driver initialization runs before the first
    // fixed tick, so use the shared game clock here.
    s.putter.initializePathSamples(s.guide,s.clock.totalFixedTickCount);
    s.input.clearFrame();
    s.players.reset();
    s.widgets.loadAll(guiAssetServices(s.resources.mounts()));
    s.roomClient=new(std::nothrow) iRoomClient;
    if(s.bindRoomClient && s.roomClient) s.bindRoomClient(s.context,*s.roomClient);
    // Publish the actual objects already assembled by this initializer.
    // AI/camera/editor-renderer slots remain the application's
    // responsibility; do not substitute unrelated objects for those globals.
    auto& values=s.variables;
    values.viewport=&s.callbacks.services.viewport;
    values.set("rnd",&s.firstRandom); values.set("fluffRnd",&s.secondRandom);
    values.set("driver",this); values.set("config",&s.config); values.set("gui",&s.gui);
    values.set("console",&s.console); values.set("sounds",&s.sounds);
    values.set("scene",&gameSceneRoots);
    values.set("font1",s.primaryFont); values.set("font2",s.consoleFont); values.set("font3",s.secondaryFont);
    values.set("fontRenderer",&editorFontRenderer);
    auto& tasks=s.callbacks.services.script.holeServices.runtime.tasks;
    values.set("tasks",&tasks.scheduledTasks); values.set("balls",&s.balls); values.set("players",&s.players);
    values.set("putter",&s.putter); values.set("hole",&s.callbacks.services.hole);
    // The original _AI_p global is only an opaque SWIG receiver: AI_cmd_impl
    // ignores it and operates on process-wide game state. Bind a stable,
    // correctly tagged command port instead of inventing a second AI object.
    values.set("ai",&bindings::aiCommandPort);
    values.set("powerups",&s.powerUpsRuntime.powers);
    values.set("objects",&s.callbacks.services.script.holeServices.runtime.objects);
    bindings::bindGameVariables(&values); bindings::bindSoundServices(&s.sounds);
    bindings::bindConfigTaskServices(&s.resources.mounts(),&tasks);
    values.set("camera",&s.callbacks.services.audioCamera);
    configureGamePython();
    Py_Initialize();
    installMountedPythonImporter();
    // netc_serviceEnd_stub is empty in the original.
    changeScript("startup",s.callbacks.services.script);
    runtime.setLegacyWindowFlag(1);
    platform::bindDriverCallbacks(s.callbacks);
    runtime.registerJoystick(true,0);
}
}
