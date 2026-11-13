// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "player_ai.h"
#include <cstdint>
namespace megusta {
struct SceneObjectRecord;
class Random;
class PuttMap;
struct PuttMapWaypoint {
    std::int16_t positionTenths[3];
    std::uint16_t shotCount, shotStart, remainingDistanceTenths;
};
struct PuttMapPackedShot { std::uint8_t headingByte,powerByte,strokeCount,flagsAndCondition; };
struct PuttMapZone {
    std::uint32_t flags;
    float center[3],radius,target[3],remainingDistance,heading,headingJitter,power,powerJitter;
    int minimumStrokes,maximumStrokes;
    std::uint32_t weight;
    int conditionIndex;
};
struct PuttMapConditionServices {
    void* context;
    std::uint32_t tick;
    SceneObjectRecord* (*findObject)(void*,const char*);
    void (*fatal)(void*);
};
struct PuttMapIO {
    void* context;
    void* (*open)(void*,const char*,const char*);
    unsigned (*read)(void*,void*,unsigned,unsigned,void*);
    char* (*getLine)(void*,char*,int,void*);
    void (*close)(void*,void*);
    void (*reportError)(void*,const char*,const char*);
    void (*fatal)();
};
struct PuttSelectionServices {
    void* context;
    Random& random;
    void (*estimate)(void*,const float*,const float*,float*,float*,bool);
    void (*fatal)(void*);
};
struct PuttScriptState { std::int8_t strokeCounts[16]{}; int count=0,firstZone=0; };
struct PuttScriptServices {
    void* context;
    PuttScriptState& state;
    PuttMap& globalMap;
    bool (*hazardsEnabled)(void*);
    bool (*cupCenter)(void*,const char*,float*);
    const char* commentMarker;
    void (*fatal)();
};
void puttMapReadExact(void* destination,int size,int count,void* file,const PuttMapIO& io);
class PuttMap {
public:
    std::uint32_t loadTick=0;
    int waypointCount=0,waypointCapacity=0,secondaryRecordCount=0,secondaryRecordCapacity=0;
    int shotCount=0,shotCapacity=0,conditionCount=0,conditionCapacity=0,zoneCount=0,zoneCapacity=0;
    PuttMapWaypoint* waypoints=nullptr;
    std::uint8_t (*secondaryRecords)[12]=nullptr;
    PuttMapPackedShot* shots=nullptr;
    PuttCondition* conditions=nullptr;
    PuttMapZone* zones=nullptr;
    void releaseData();
    void allocateData(int waypointCapacity,int secondaryCapacity,int shotCapacity,int conditionCapacity,void (*fatal)());
    void appendZone(const PuttMapZone* zone,void (*fatal)());
    bool testCondition(const PuttCondition* condition,const PuttMapConditionServices& services) const;
    long double estimateRemainingDistance(const float* position,float fallback) const;
    bool loadBinary(const char* holeName,std::uint32_t tick,const PuttMapIO& io);
    void selectPutt(int strokes,const float* position,PuttShotChoice& choice,const PuttSelectionServices& services) const;
    void parseStatement(const char* line,const PuttScriptServices& services);
    void loadScript(const char* holeName,const PuttMapIO& io,const PuttScriptServices& services);
};
static_assert(sizeof(PuttMapWaypoint)==12 && sizeof(PuttMapPackedShot)==4 && sizeof(PuttMapZone)==68 && sizeof(PuttCondition)==84,"Putt map record widths");
}
