#pragma once
#include "putt_map.h"
namespace megusta {
class GameConfig;
struct SceneObjectCollection;
long double findNearestCup(SceneObjectCollection&,const float* position,float* outputPosition,
                           int* outputIndex,bool horizontalOnly,void (*fatal)(void*),void* context);
struct PuttEstimateServices {
    SceneObjectCollection& objects;
    Random& random;
    void* context;
    const float* (*currentBallPosition)(void*);
    void (*fatal)(void*);
};
void estimatePutt(const float* position,const float* target,float* heading,float* power,bool randomized,
                  const PuttEstimateServices& services);
struct PuttMapRuntimeHost {
    PuttMapIO& io;
    void* context;
    const float* (*currentBallPosition)(void*);
    void (*fatal)(void*);
};
// Referenced state and host services must outlive callbacks into this adapter.
class PuttMapRuntime {
public:
    PuttMapRuntime(PuttMap&,SceneObjectCollection&,GameConfig&,Random&,PuttMapRuntimeHost&);
    void load(const char* holeName,std::uint32_t tick);
    bool loadBinary(const char* holeName,std::uint32_t tick);
    void loadScript(const char* holeName);
    void release();
    void loadCustomScript(const char*);
    void parseCustomStatement(const char*);
    bool testCondition(const PuttCondition*,std::uint32_t tick);
    bool updatePlayerDecision(Player&,bool cameraControlMode,float& cameraHeading,std::uint32_t tick);
    static void selectForPlayerAI(void*,int,const float*,int,PuttShotChoice&);
    PuttMap& map;
    SceneObjectCollection& objects;
    GameConfig& config;
    Random& random;
    PuttMapRuntimeHost& host;
    PuttScriptState scriptState{};
};
}
