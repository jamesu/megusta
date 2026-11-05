#include "putt_map.h"
#include "random.h"
#include "scene_grid.h"
#include "recovered_binding_primitives.h"
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace megusta {
void PuttMap::selectPutt(int strokes,const float* p,PuttShotChoice& result,const PuttSelectionServices& s) const {
    result={};
    result.heading=static_cast<float>(s.random.generate()%1000)*0.36f;
    result.power=static_cast<float>(s.random.generate()%500)*0.001f+0.5f;
    s.estimate(s.context,p,nullptr,&result.fallbackHeading,&result.fallbackPower,false);
    if (s.random.generate()&3) s.estimate(s.context,p,nullptr,&result.heading,&result.power,true);
    auto& scratch=*reinterpret_cast<PuttSelectionScratch*>(&puttSelectionWorkspace);
    std::memset(&scratch,0,sizeof scratch);
    auto add=[&](std::uint32_t weight,int index) {
        if (scratch.count>=1024 || static_cast<std::int32_t>(weight)<1) { s.fatal(s.context); return; }
        scratch.totalWeight+=weight;
        scratch.entries[scratch.count++]={weight,index};
    };
    auto pick=[&]() {
        if (scratch.count<1 || static_cast<std::int32_t>(scratch.totalWeight)<1) { s.fatal(s.context); return 0; }
        std::uint32_t value=scratch.totalWeight<2 ? 0 : s.random.generate()%scratch.totalWeight;
        for (int i=0;i<scratch.count;++i) {
            value-=scratch.entries[i].weight;
            if (static_cast<std::int32_t>(value)<0) return scratch.entries[i].index;
        }
        return scratch.entries[scratch.count-1].index;
    };
    for (int i=0;i<zoneCount;++i) {
        const auto& z=zones[i];
        if (strokes<z.minimumStrokes || strokes>z.maximumStrokes) continue;
        float x=z.center[0]-p[0],y=z.center[1]-p[1],v=z.center[2]-p[2];
        if (z.flags&0x20) v=0;
        if (binding_math_sqrtf(y*y+v*v+x*x)<z.radius) add(z.weight,i);
    }
    if (scratch.count) {
        const auto& z=zones[pick()];
        if (!(z.flags&4)) {
            result.distance=0; result.estimatedStrokeCost=static_cast<float>(z.minimumStrokes);
            if (!(z.flags&8)) { result.kind=1; result.heading=z.heading; result.power=z.power; }
            else {
                float target[3]={z.target[0],z.target[1],(z.flags&0x20)?p[2]:z.target[2]};
                s.estimate(s.context,p,target,&result.heading,&result.power,false); result.kind=3;
            }
            if (z.headingJitter>0) result.heading+=(static_cast<float>(s.random.generate()%2000)-1000.0f)*0.001f*z.headingJitter;
            if (z.powerJitter>0) result.power+=(static_cast<float>(s.random.generate()%2000)-1000.0f)*0.001f*z.powerJitter;
            if (result.power>=0.99f) result.power=0.99f; else if (result.power<=0.01f) result.power=0.01f;
            if (z.conditionIndex>0) {
                if (conditionCount<=z.conditionIndex) { s.fatal(s.context); return; }
                result.hasCondition=true; result.condition=conditions[z.conditionIndex];
            }
            return;
        }
    }
    const PuttMapWaypoint* nearest=nullptr; float best=FLT_MAX,dx=0,dy=0,dz=0;
    for (int i=0;i<waypointCount;++i) {
        const auto& w=waypoints[i];
        float x=static_cast<float>(w.positionTenths[0])*0.1f-p[0];
        float y=static_cast<float>(w.positionTenths[1])*0.1f-p[1];
        float z=static_cast<float>(w.positionTenths[2])*0.1f-p[2];
        float d=x*x+y*y+z*z;
        if (d<best) { best=d; nearest=&w; dx=x; dy=y; dz=z; }
    }
    if (!nearest || std::fabs(dz)>16.0f) return;
    for (int delta=0;delta<=2;++delta) {
        std::memset(&scratch,0,sizeof scratch);
        for (int i=0;i<nearest->shotCount;++i) {
            const auto& shot=shots[nearest->shotStart+i];
            if ((shot.strokeCount==static_cast<std::uint8_t>(strokes+delta) || shot.strokeCount==static_cast<std::uint8_t>(strokes-delta)) && !(shot.flagsAndCondition&15)) add(1,i);
        }
        if (scratch.count) break;
    }
    if (!scratch.count) return;
    const auto& shot=shots[nearest->shotStart+pick()];
    result.kind=2; result.heading=static_cast<float>(shot.headingByte)*1.40625f;
    result.power=static_cast<float>(shot.powerByte)*0.003921569f;
    result.distance=binding_math_sqrtf(dz*dz+dy*dy+dx*dx);
    result.estimatedStrokeCost=static_cast<float>(shot.strokeCount);
    unsigned condition=shot.flagsAndCondition>>4;
    if (condition) {
        if (condition>=static_cast<unsigned>(conditionCount)) { s.fatal(s.context); return; }
        result.hasCondition=true; result.condition=conditions[condition];
    }
}
}
