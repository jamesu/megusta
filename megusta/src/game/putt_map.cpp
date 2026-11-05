#include "putt_map.h"
#include "scene_object.h"
#include "recovered_binding_primitives.h"
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
namespace megusta {
void puttMapReadExact(void* destination,int size,int count,void* file,const PuttMapIO& io) {
    if (!size || !count) return;
    if (!destination || !file || size<1 || count<1) { io.fatal(); return; }
    if (io.read(io.context,destination,static_cast<unsigned>(size),static_cast<unsigned>(count),file)!=static_cast<unsigned>(count)) {
        io.fatal();
        std::memset(destination,0,static_cast<std::size_t>(size)*count);
    }
}
bool PuttMap::loadBinary(const char* holeName,std::uint32_t tick,const PuttMapIO& io) {
    releaseData();
    char path[128];
    std::snprintf(path,sizeof(path),"data/AI/PUTT_%s.DAT",holeName);
    void* file=io.open(io.context,path,"rb");
    if (!file) { io.reportError(io.context,"Unable to open AI (%s)",holeName); return false; }
    std::int32_t id=0,version=0,unused=0,nw=0,ns=0,np=0,nc=0;
    puttMapReadExact(&id,4,1,file,io);
    puttMapReadExact(&version,4,1,file,io);
    if (id!=0x54545550 || version!=2) {
        io.reportError(io.context,"Invalid AI table for (%s)",holeName);
        io.close(io.context,file); return false;
    }
    puttMapReadExact(&unused,4,1,file,io);
    loadTick=tick;
    puttMapReadExact(&nw,4,1,file,io); puttMapReadExact(&ns,4,1,file,io);
    puttMapReadExact(&np,4,1,file,io); puttMapReadExact(&nc,4,1,file,io);
    // The recovered allocateData releases and zeroes the map after loadTick is set.
    allocateData(nw,ns,np,nc,io.fatal);
    waypointCount=nw; secondaryRecordCount=ns; shotCount=np; conditionCount=nc;
    if (nw) puttMapReadExact(waypoints,12,nw,file,io);
    if (ns) puttMapReadExact(secondaryRecords,12,ns,file,io);
    if (np) puttMapReadExact(shots,4,np,file,io);
    if (nc) puttMapReadExact(conditions,84,nc,file,io);
    puttMapReadExact(&id,4,1,file,io);
    if (id!=0x54545550) io.fatal();
    io.close(io.context,file); return true;
}
void PuttMap::releaseData() {
    std::free(waypoints); std::free(secondaryRecords); std::free(shots);
    std::free(conditions); std::free(zones);
    *this=PuttMap{};
}
void PuttMap::allocateData(int nw,int ns,int np,int nc,void (*fatal)()) {
    releaseData();
    if (nw>0) { waypointCapacity=nw; waypoints=static_cast<PuttMapWaypoint*>(std::malloc(static_cast<std::size_t>(nw)*12)); if (!waypoints) fatal(); }
    if (ns>0) { secondaryRecordCapacity=ns; secondaryRecords=static_cast<std::uint8_t (*)[12]>(std::malloc(static_cast<std::size_t>(ns)*12)); if (!secondaryRecords) fatal(); }
    if (np>0) { shotCapacity=np; shots=static_cast<PuttMapPackedShot*>(std::malloc(static_cast<std::size_t>(np)*4)); if (!shots) fatal(); }
    if (nc>0) { conditionCapacity=nc; conditions=static_cast<PuttCondition*>(std::malloc(static_cast<std::size_t>(nc)*84)); if (!conditions) fatal(); }
}
void PuttMap::appendZone(const PuttMapZone* zone,void (*fatal)()) {
    if (zoneCapacity<=zoneCount) {
        zoneCapacity+=64;
        auto* next=static_cast<PuttMapZone*>(std::calloc(zoneCapacity,sizeof(PuttMapZone)));
        if (!next) { fatal(); return; }
        if (zones) { std::memcpy(next,zones,zoneCount*sizeof(PuttMapZone)); std::free(zones); }
        zones=next;
    }
    if (!zone) { fatal(); return; }
    zones[zoneCount++]=*zone;
}
bool PuttMap::testCondition(const PuttCondition* condition,const PuttMapConditionServices& s) const {
    if (!condition) { s.fatal(s.context); return true; }
    const char* name=reinterpret_cast<const char*>(condition);
    if (!*name) return true;
    std::uint32_t value;
    if (std::strcmp(name,"__TICK_COUNT__")==0) value=s.tick-loadTick;
    else {
        SceneObjectRecord* object=s.findObject(s.context,name);
        if (!object) { s.fatal(s.context); return true; }
        value=object->currentFrame;
    }
    int signedValue=static_cast<std::int32_t>(value);
    if (condition->modulus>1) signedValue%=condition->modulus;
    bool result=signedValue>=condition->minimum && signedValue<=condition->maximum;
    return condition->invert ? !result : result;
}
long double PuttMap::estimateRemainingDistance(const float* p,float fallback) const {
    for (int i=0;i<zoneCount;++i) {
        const auto& z=zones[i];
        if (!(z.flags&0x11)) continue;
        float x=z.center[0]-p[0],y=z.center[1]-p[1],v=z.center[2]-p[2];
        if (z.flags&0x20) v=0;
        if (binding_math_sqrtf(y*y+v*v+x*x)<=z.radius) {
            x=z.target[0]-p[0]; y=z.target[1]-p[1]; v=z.target[2]-p[2];
            return static_cast<long double>(binding_math_sqrtf(y*y+v*v+x*x))+z.remainingDistance;
        }
    }
    const PuttMapWaypoint* nearest=nullptr; float best=FLT_MAX,nearestX=0,nearestY=0,nearestZ=0;
    for (int i=0;i<waypointCount;++i) {
        const auto& w=waypoints[i];
        float x=static_cast<float>(w.positionTenths[0])*0.1f-p[0];
        float y=static_cast<float>(w.positionTenths[1])*0.1f-p[1];
        float z=static_cast<float>(w.positionTenths[2])*0.1f-p[2];
        float distance=x*x+y*y+z*z;
        if (distance<best) { best=distance; nearest=&w; nearestX=static_cast<float>(w.positionTenths[0])*0.1f; nearestY=static_cast<float>(w.positionTenths[1])*0.1f; nearestZ=static_cast<float>(w.positionTenths[2])*0.1f; }
    }
    if (nearest) {
        // The saved body calls sqrtf for the nearest waypoint and discards its
        // result before forming this return. Preserve the call and its FP
        // status effects even though it does not contribute to the value.
        (void)binding_math_sqrtf((nearestX-p[0])*(nearestX-p[0])+
            (nearestZ-p[2])*(nearestZ-p[2])+(nearestY-p[1])*(nearestY-p[1]));
        return static_cast<long double>(fallback)*0.001L+static_cast<float>(nearest->remainingDistanceTenths)*0.1f;
    }
    return fallback;
}
}
