#pragma once
#include "gameplay_runtime.h"
#include "game_config_runtime.h"
#include "input.h"
namespace megusta {
class PuttMapRuntime;
struct HoleFrameClock {
    std::uint32_t transitionStartTick,transitionDurationTicks,fixedTicksThisFrame;
    float framesPerSecond;
    std::uint32_t nextFixedTickDeadlineMs;
    float fixedTicksPerSecond;
    std::uint32_t previousFrameCount,previousTickCount,rateWindowStartMs,totalFixedTickCount,totalRenderedFrameCount;
    void resetFrameCounters(std::uint32_t now);
    void sampleFrameRates(std::uint32_t now);
    void resetFrameDeadline(std::uint32_t now);
    bool advanceFixedTick(std::uint32_t now);
};
enum class HoleAction {
    releasePuttMap,saveSpatialObjects,releaseCamera,releaseBallModels,releasePutter,
    releaseTransientSounds,releaseEffects,serviceNetwork,releasePowerUps,
    initializeSceneRoot,buildSceneObjects,loadBallModels,loadPutter,
    initializeCamera,initializeEffects,initializeWireDebug,loadPowerUps,loadSpatialObjects,
    loadPuttMapBinary,loadPuttMapScript,putterInput,wireInput,powerUpInput,cameraInput,
    spatialEditorInput,advancePutter,updateEffects,processPickups,applyCameraInput,
    drawBalls,updateCameraFollow,drawAimPreview,collectWireCandidates,drawCharacterPass1,
    drawSceneRoot,drawGroundShadows,drawCharacterPass2,drawSpatialDebug,advancePowerAnimations,
    drawLensEffects,drawCameraMarker,drawCharacterOcclusion,drawPowerHud,drawPlayersOverlay,
    drawPowerEditorPosition,drawEffects,drawSelectedSpatialDebug
};
// Arguments to the render callbacks are the recovered engine adapter tokens,
// which the eventual GL backend resolves to platform API values.
struct HoleRenderServices {
    void* context;
    void (*matrixMode)(void*,int);
    void (*loadIdentity)(void*);
    void (*projectionIdentity)(void*);
    void (*depthRange)(void*,double,double);
    void (*colorMask)(void*,bool,bool,bool,bool);
    void (*disable)(void*,int);
    void (*enable)(void*,int);
    void (*polygonMode)(void*,int,int);
    void (*frontFace)(void*,int);
    void (*disableClientState)(void*,int);
    void (*blendFunc)(void*,int,int);
    void (*texEnvi)(void*,int,int,int);
    void (*depthMask)(void*,bool);
    void (*color4f)(void*,float,float,float,float);
    void (*shadeModel)(void*,int);
    void (*clearColor)(void*,float,float,float,float);
    void (*clearFlags)(void*,int);
    void (*depthFunc)(void*,int);
    void (*viewport)(void*,int,int,int,int);
    void (*ortho)(void*,double,double,double,double,double,double);
};
struct HoleServices {
    GameplayRuntime& runtime;
    PuttMapRuntime& puttMap;
    const char* currentScriptName;
    GameInputFrame& input;
    InputState& inputState;
    const float* smoothedAim;
    void* context;
    void (*action)(void*,HoleAction);
    void (*setAmbientColour)(void*,float,float,float);
    bool (*isEditing)(void*);
    bool (*canAdvanceSimulation)(void*);
    GameConfigRuntime& config;
    HoleRenderServices& render;
};
class Hole {
public:
    char* ownedInitString0;
    char* ownedInitString1;
    int initIntegerArgument;
    float teePositions[3][3];
    int transitionState;
    void clearGlobalPrefix();
    bool hasActiveScene(const SceneObjectCollection& objects) const;
    bool isTransitioning(const SceneObjectCollection& objects) const;
    void beginTransition(HoleFrameClock& clock);
    void releaseResources(HoleServices& services);
    void initialize(const char* first,const char* second,int argument,HoleFrameClock& clock,HoleServices& services);
    void update(HoleFrameClock& clock,HoleServices& services);
    void processFrame(HoleFrameClock& clock,HoleServices& services);
    void draw(HoleServices& services);
    void drawHud(HoleServices& services);
};
void setupHoleHudRenderState(HoleRenderServices& render);
extern Hole gameHole;
extern HoleFrameClock gameHoleClock;
void Hole_global_initialize_impl();
}
