#include "putt_map_runtime.h"
#include "scene_object.h"
#include "game_config.h"
#include "player.h"
#include "ai_diagnostics.h"
#include <cstdio>
#include <cstring>
namespace megusta {
namespace {
PuttMapRuntime& runtime(void* p) { return *static_cast<PuttMapRuntime*>(p); }
SceneObjectRecord* findObject(void* p,const char* name) { return runtime(p).objects.findByName(name); }
bool hazards(void* p) { return runtime(p).config.readBool("Game","hazards",false); }
bool cupCenter(void* p,const char* name,float* center) {
    auto* object=findObject(p,name);
    if (!object || !object->entity) return false;
    const float* position=object->entity->getBSphere()->getCenter();
    std::memcpy(center,position,3*sizeof(float)); return true;
}
void fatal(void* p) { auto& r=runtime(p); r.host.fatal(r.host.context); }
void estimate(void* p,const float* from,const float* to,float* heading,float* power,bool randomized) {
    auto& r=runtime(p);
    PuttEstimateServices services{r.objects,r.random,r.host.context,r.host.currentBallPosition,r.host.fatal};
    estimatePutt(from,to,heading,power,randomized,services);
}
}
PuttMapRuntime::PuttMapRuntime(PuttMap& m,SceneObjectCollection& o,GameConfig& c,Random& r,PuttMapRuntimeHost& h)
    : map(m),objects(o),config(c),random(r),host(h) {}
void PuttMapRuntime::load(const char* holeName,std::uint32_t tick) {
    const bool loaded=loadBinary(holeName,tick);
    loadScript(holeName);
    if(aiDiagnosticsEnabled()) std::printf("AI course=%s table=%s waypoints=%d shots=%d zones=%d conditions=%d\n",
        holeName,loaded?"loaded":"missing/invalid",map.waypointCount,map.shotCount,map.zoneCount,map.conditionCount);
}
bool PuttMapRuntime::loadBinary(const char* holeName,std::uint32_t tick) {
    return map.loadBinary(holeName,tick,host.io);
}
void PuttMapRuntime::loadScript(const char* holeName) {
    PuttScriptServices services{this,scriptState,map,hazards,cupCenter,"//",host.io.fatal};
    map.loadScript(holeName,host.io,services);
}
void PuttMapRuntime::release() { map.releaseData(); }
void PuttMapRuntime::loadCustomScript(const char* name) {
    PuttScriptServices services{this,scriptState,map,hazards,cupCenter,"//",host.io.fatal}; map.loadScript(name,host.io,services);
}
void PuttMapRuntime::parseCustomStatement(const char* text) {
    PuttScriptServices services{this,scriptState,map,hazards,cupCenter,"//",host.io.fatal}; map.parseStatement(text,services);
}
bool PuttMapRuntime::testCondition(const PuttCondition* condition,std::uint32_t tick) {
    PuttMapConditionServices services{this,tick,findObject,fatal};
    return map.testCondition(condition,services);
}
bool PuttMapRuntime::updatePlayerDecision(Player& player,bool cameraControlMode,float& cameraHeading,std::uint32_t tick) {
    PlayerAIUpdateServices services{this,cameraControlMode,cameraHeading,tick,random,selectForPlayerAI};
    return player.ai.updateShotDecision(player,services);
}
void PuttMapRuntime::selectForPlayerAI(void* p,int strokes,const float* position,int,PuttShotChoice& choice) {
    auto& r=runtime(p); PuttSelectionServices services{p,r.random,estimate,fatal};
    r.map.selectPutt(strokes,position,choice,services);
    if(aiDiagnosticsEnabled()) std::printf("AI shot budget=%d position=(%g,%g,%g) kind=%d heading=%g power=%g fallback=(%g,%g) waypointDistance=%g cost=%g\n",
        strokes,position[0],position[1],position[2],choice.kind,choice.heading,choice.power,
        choice.fallbackHeading,choice.fallbackPower,choice.distance,choice.estimatedStrokeCost);
}
}
