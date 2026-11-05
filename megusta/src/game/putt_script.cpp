#include "putt_map.h"
#include "scene_grid.h"
#include "../engine/string_services.h"
#include <cstdio>
#include <cstring>
namespace megusta {
void PuttMap::loadScript(const char* holeName,const PuttMapIO& io,const PuttScriptServices& s) {
    char path[128]; std::snprintf(path,sizeof(path),"data/AI/PUTT_%s.SCR",holeName);
    void* file=io.open(io.context,path,"r");
    if (!file) return;
    char line[260]{};
    while (io.getLine(io.context,line,256,file)) {
        char* comment=std::strstr(line,s.commentMarker);
        if (comment) *comment=0;
        std::size_t length=std::strlen(line);
        while (length && line[length-1]<'!') line[--length]=0;
        if (line[0]) parseStatement(line,s);
        std::memset(line,0,257);
    }
    io.close(io.context,file);
}
void PuttMap::parseStatement(const char* line,const PuttScriptServices& s) {
    auto& state=s.state;
    auto is=[&](const char* prefix,int n) { return strings::compareInsensitiveBounded(line,prefix,n)==0; };
    auto reset=[&]() { state.count=0; state.firstZone=zoneCount; std::memset(state.strokeCounts,0,sizeof(state.strokeCounts)); };
    auto increment=[&]() {
        if (++state.count>15) { s.fatal(); return; }
        for (int i=0;i<state.count;++i) ++state.strokeCounts[i];
    };
    if (is("TEE:",4)) { reset(); return; }
    if (is("CUP:",4)) {
        for (int i=0;i<state.count;++i) {
            auto& zone=zones[state.firstZone+i];
            if (zone.minimumStrokes>=0) break;
            zone.minimumStrokes=zone.maximumStrokes=state.strokeCounts[i];
        }
        reset(); return;
    }
    if (is("HIT:",4)) {
        PuttMapZone zone{}; zone.minimumStrokes=-1; zone.weight=1; zone.power=-1; zone.radius=1;
        std::sscanf(line+4,"%f,%f,%f, %f,%f, %d,%u,%d",&zone.center[0],&zone.center[1],&zone.center[2],&zone.heading,&zone.power,&zone.minimumStrokes,&zone.weight,&zone.conditionIndex);
        if (zone.power<0) { s.fatal(); return; }
        appendZone(&zone,s.fatal); increment(); return;
    }
    if (is("BALLLOST:",9)) { if (s.hazardsEnabled(s.context)) increment(); return; }
    if (is("SETCUP:",7)) {
        float radius=-1;
        auto* name=reinterpret_cast<char*>(&puttSelectionWorkspace);
        name[0]='\0';
        // The executable scans an unbounded %s directly into its shared
        // selection workspace; keep that write and aliasing behavior.
        std::sscanf(line+7,"%f %s",&radius,name);
        if (radius<=0 || std::strlen(name)==0) { s.fatal(); return; }
        if (!std::strcmp(name,"(GUESS)")) std::strcpy(name,"cup_01");
        PuttMapZone zone{};
        if (!s.cupCenter(s.context,name,zone.center)) { s.fatal(); return; }
        zone.flags=0x2b; zone.radius=radius; zone.minimumStrokes=1; zone.maximumStrokes=2; zone.weight=1;
        std::memcpy(zone.target,zone.center,sizeof(zone.target));
        s.globalMap.appendZone(&zone,s.fatal); return;
    }
    if (is("ADDZONE:",8)) {
        PuttMapZone zone{}; zone.maximumStrokes=-1; zone.weight=1; zone.flags=0x1a;
        int weight=static_cast<int>(zone.weight);
        std::sscanf(line+8,"%f,%f,%f,%f, %f,%f,%f,%f, %d,%d, %f,%f,%d",&zone.center[0],&zone.center[1],&zone.center[2],&zone.radius,&zone.target[0],&zone.target[1],&zone.target[2],&zone.remainingDistance,&zone.minimumStrokes,&zone.maximumStrokes,&zone.headingJitter,&zone.powerJitter,&weight);
        zone.weight=static_cast<std::uint32_t>(weight);
        if (zone.maximumStrokes<0) { s.fatal(); return; }
        appendZone(&zone,s.fatal); return;
    }
    if (is("ADDSTUB:",9)) {
        PuttMapZone zone{}; zone.maximumStrokes=-1; zone.flags=4;
        int weight=0;
        std::sscanf(line+9,"%d,%d,%d",&weight,&zone.minimumStrokes,&zone.maximumStrokes);
        zone.weight=static_cast<std::uint32_t>(weight);
        if (zone.maximumStrokes<0) zone.maximumStrokes=zone.minimumStrokes;
        if (static_cast<std::int32_t>(zone.weight)<0) { s.fatal(); return; }
        appendZone(&zone,s.fatal);
    }
}
}
