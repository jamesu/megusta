#include "startup.h"
#include "driver_initialize.h"
#include "audio/openal.h"
#include "engine/movie_playback.h"
#include "startup_literals.h"
#include "engine/case_insensitive_path.h"
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <sys/stat.h>
namespace megusta {
namespace {
char lower(char c) { return c>='A' && c<='Z'?char(c+('a'-'A')):c; }
bool prefix(const char* value,const char* key) {
    while (*key) if (lower(*value++)!=lower(*key++)) return false;
    return true;
}
bool equal(const char* value,const char* key) {
    while (*key) if (lower(*value++)!=lower(*key++)) return false;
    return *value==0;
}
}
// 00402360 argument branch. Apply each occurrence in order: autorun and camera
// setters are observable calls, not just last-value configuration fields.
void parseStartupArguments(int argc,char* const* argv,StartupState& state,const StartupServices& host) {
    state.intro=true;
    for (int i=1;i<argc;++i) {
        const char* arg=argv[i];
        if (prefix(arg,"-sceneLoadMethod:")) state.sceneLoadMethod=std::atoi(arg+17);
        else if (equal(arg,"-forcepix")) state.forcePix=true;
        else if (equal(arg,"-buildtbl")) state.buildTables=true;
        else if (equal(arg,"-demo")) state.demo=true;
        else if (prefix(arg,"-memdebug:")) {} // The original branch is empty.
        else if (equal(arg,"-nointro")) state.intro=false;
        else if (prefix(arg,"-autorun:")) {
            int menu=0,game=0;
            std::sscanf(arg+9,"%d,%d",&menu,&game);
            host.autorun(host.context,menu,game,false);
        } else if (prefix(arg,"-camdelay:")) host.cameraDelay(host.context,std::atoi(arg+10));
        else if (prefix(arg,"-alpha:")) state.alpha=std::atoi(arg+7);
    }
}
// 00402360, reached through the 00401000 startup bridge. Platform-specific
// argument mutation, movie services and Driver assembly remain host bindings.
int runGameStartup(int argc,char** argv,ResourceRuntime& resources,GameConfig& config,
    DriverViewport& viewport,platform::Runtime& platform,StartupState& state,DriverInitialization& driver,const StartupServices& host) {
    if (&driver.callbacks.runtime!=&platform || &driver.startup!=&state || !host.shellConfig || !host.autorun || !host.cameraDelay ||
        !host.applyPreferences)
        throw std::logic_error("Game startup requires unresolved native service bindings");
    struct stat status{};
    std::string configPath;
    if (!engine_filesystem::resolvePathCaseInsensitive("MINIGOLF.CFG", configPath) ||
        ::stat(configPath.c_str(),&status)!=0)
        host.shellConfig(host.context,"MinigolfConfig.exe");
    resources.initializeFromArgs(argc,argv);
    auto& mounts=resources.mounts();
    static const struct { const char* prefix; const char* path; } fixedMounts[]={
        {"data/","base"},{"data/","scripts.rbx"},
        // Emscripten's preloaded filesystem is case-sensitive; the shipped
        // archive is named ai.rbx (Windows accepts either spelling).
        {"data/AI/","ai.rbx"},
        {"data/announce/","announce.rbx"},{"data/coins/","coins.rbx"},
        {"data/fonts/","fonts.rbx"},{"data/shapes/","shapes.rbx"},
        {"data/sounds/","sounds.rbx"},{"data/textures/","textures.rbx"},
        {"data/voices/","voices.rbx"},{"music/","music"},{"music/","music.rbx"}
    };
    for (const auto& mount:fixedMounts) mounts.add(mount.prefix,mount.path);
    mounts.add("/mnt/memcard/",startup_literals::resourcePath);
    mounts.add("/tmp/",startup_literals::resourcePath);
    parseStartupArguments(argc,argv,state,host);
    config.load(mounts);
    viewport.displayWidth=config.readInt("Prefs","ScreenWidth",640);
    viewport.displayHeight=(viewport.displayWidth*480)/640;
    auto& prefs=state.preferences;
    prefs.screenDepth=config.readInt("Prefs","DepthScreen",prefs.screenDepth);
    prefs.zDepth=config.readInt("Prefs","DepthZBuff",prefs.zDepth);
    prefs.soundDepth=config.readInt("Prefs","DepthSound",prefs.soundDepth);
    audio::requestedBits=prefs.soundDepth;
    prefs.textureDepth=config.readInt("Prefs","DepthTexture",prefs.textureDepth);
    prefs.textureLimitIndex=config.readInt("Prefs","TextureLimit",prefs.textureLimitIndex+1)-1;
    prefs.vsyncMode=config.readInt("Prefs","VSyncMode",prefs.vsyncMode);
    host.applyPreferences(host.context,prefs);
    if (config.readInt("Prefs","FullScreen",0)!=0) platform.requestInitialFullscreen();
    viewport.set(-1,-1,false);
    platform.state.colorBits=prefs.screenDepth;
    platform.state.depthBits=prefs.zDepth;
    platform.initializeFromArgs(argc,argv);
    prefs.screenDepth=platform.state.colorBits; // -bitdepth overrides the loaded preference.
    platform.setInitialSize(viewport.displayWidth,viewport.displayHeight);
    // Keep the configured 4:3 display dimensions as the gameplay canvas.
    // Menu scripts select their separate 640x480 viewport. SDL fits either
    // logical canvas into the current drawable without changing these values.
    platform.create("Minigolf Maniacs"); // Original caller ignores its result.
    if (host.movieCleanup) host.movieCleanup(host.context,"Minigolf Maniacs",0x12); else engine_movie::registerMovieCleanup();
    audio::initialize(&argc,argv);
    audio::distanceScale(0.04445009f);
    driver.callbacks.driver.initialize(driver);
    const auto movie=[&](const char* name,int flags,int x,int y,int width,int height) {
        return host.playMovie?host.playMovie(host.context,name,flags,x,y,width,height):engine_movie::playMovie(name,flags,x,y,width,height);
    };
    if (state.intro) {
        int x=(viewport.displayWidth-640)/2,y=(viewport.displayHeight-480)/2;
        if (movie("movies/sierra",2,x,y,640,480)==0 && movie("movies/3dultra",2,x,y,640,480)==0)
            movie("movies/intro",2,x,y+40,640,400);
    }
    platform.runMainLoop();
    return 0;
}
}
