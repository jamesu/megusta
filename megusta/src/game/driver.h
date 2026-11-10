#pragma once
#include "hole.h"
#include "engine/runtime_adapters.h"
#include "platform/surface_transform.h"
namespace megusta {
struct DriverViewport {
    int displayWidth,displayHeight,viewportWidth,viewportHeight,offsetX,offsetY;
    bool scaled;
    platform::SurfaceTransform surface;
    platform::PixelRectangle pixelRectangle()const;
    platform::PixelRectangle pixelScissor(int x,int y,int width,int height)const;
    void set(int width,int height,bool scale);
    void mapMouse(int& x,int& y) const;
    void unmapMouse(int& x,int& y) const;
};
class Driver;
struct DriverScriptServices;
struct DriverInitialization;
class Sounds;
class Camera;
class guiSystem;
class iRoomClient;
class Console;
struct GuiRuntime;
struct GuiDebugState;
struct DriverAutorunServices {
    GameConfigRuntime& config;
    Hole& hole;
    PlayersManager& players;
    InputState& input;
    void* context;
    std::uint32_t (*timeMilliseconds)(void*);
};
class DriverAutorun {
public:
    bool enabled;
    int menuSeconds,gameSeconds;
    std::uint32_t deadline,updateCounter;
    int lastMouseX,lastMouseY;
    void configure(int menu,int game,bool startNow,const DriverAutorunServices& services);
    bool inputGate(Driver& driver,DriverScriptServices& scripts,DriverAutorunServices& services);
    void update(bool force,Driver& driver,DriverScriptServices& scripts,DriverAutorunServices& services);
    void restoreConfig(const char* name,Driver& driver,DriverScriptServices& scripts,DriverAutorunServices& services);
};
struct DriverScriptServices {
    HoleServices& holeServices;
    guiSystem& gui;
    iRoomClient*& client;
    Sounds& sounds;
    void* context;
    void (*fatal)(void*);
    DriverAutorun& autorun;
    DriverAutorunServices& autorunServices;
    const char* argumentDelimiters;
};
struct DriverInputServices {
    InputState& input;
    const InputServices& inputServices;
    DriverViewport& viewport;
    Hole& hole;
    PlayersManager& players;
    GameConfigRuntime& config;
    DriverScriptServices& scripts;
    void* context;
    int (*joystickDeviceId)(void*);
    void (*consoleButtons)(void*,std::uint32_t);
    void (*consoleSpecialKey)(void*,int);
    void (*consoleCharacter)(void*,std::uint8_t);
    bool (*consoleVisible)(void*);
    bool (*consoleOpening)(void*);
    void (*disableWarpCursorOnTab)(void*);
    void (*puiMouseButton)(void*,int,int,int,int);
    void (*puiMouseMotion)(void*,int,int);
    void (*puiKeyboard)(void*,std::uint32_t,int);
};
struct DriverServices {
    Hole& hole;
    HoleFrameClock& clock;
    HoleServices& holeServices;
    DriverViewport& viewport;
    DriverScriptServices& script;
    bool& slowMotion;
    int& slowMotionTickDivider;
    guiSystem& gui;
    GuiRuntime& guiState;
    GuiDebugState& guiDebug;
    Console& console;
    iRoomClient*& client;
    Sounds& sounds;
    Camera& audioCamera;
    void* context;
    bool (*shellOpen)(void*);
    bool (*hasBackground)(void*);
    bool (*hasSoftwareCursor)(void*);
    bool (*windowCursorEnabled)(void*);
    bool (*hasTabElements)(void*);
    bool (*coordinateTurnReadiness)(void*);
    bool (*cameraCollisionEnabled)(void*);
    std::uint32_t (*timeMilliseconds)(void*);
    void (*sleepMicroseconds)(void*,std::uint32_t);
    void (*displayWidgets)(void*,int,int);
    void (*dispatchJoystick)(void*);
};
class Driver {
public:
    void initialize(DriverInitialization&);
    char activeScript[80],pendingScript[80];
    bool fpsOverlayEnabled,frameLimitEnabled;
    engine_runtime::ExitHandler previousExitHandler;
    bool clearNextFrame;
    std::uint32_t previousIdleTime;
    char lastScript[80];
    float joystickAggregateAxes[4];
    std::uint32_t joystickAggregateButtons;
    void setup3dRenderState(HoleRenderServices& render);
    void setup2dProjection(DriverViewport& viewport,HoleRenderServices& render);
    void frame(DriverServices& services);
    void idleUpdate(DriverServices& services);
    static void executePython(const char* command);
    void changeScript(const char* script,DriverScriptServices& services);
    void reloadScript(DriverScriptServices& services);
    void applyPendingScript(DriverScriptServices& services);
    void joystickCallback(std::uint32_t buttons,int count,const float* axes,DriverInputServices& services);
    void mouseButtonCallback(int button,int state,int x,int y,DriverInputServices& services);
    void mouseMotionCallback(int x,int y,DriverInputServices& services);
    void specialKeyDownCallback(int key,DriverInputServices& services);
    void specialKeyUpCallback(int key,DriverInputServices& services);
    void keyDownCallback(std::uint8_t key,DriverInputServices& services);
};
extern Driver gameDriver;
extern DriverViewport driverViewport;
extern DriverAutorun driverAutorun;
}
