#include "ground_shadow.h"
#include "engine/opengl_adapter.h"
#include <cstdlib>
#include <cstring>
namespace megusta {
GroundShadowCacheState groundShadowCache;
ShapeShadowMaskStorage shapeShadowMask;
bool lookupOrBeginGroundShadow(float x0,float x1,float y0,float y1) {
    auto& cache=groundShadowCache;
    for (auto& entry:cache.entries) {
        float listBits;
        std::memcpy(&listBits,&entry.displayList,sizeof(listBits));
        if (entry.parameters[0]==x0 && entry.parameters[1]==x1 && entry.parameters[2]==y0 &&
            entry.parameters[3]==y1 && listBits!=0.0f) { cache.current=&entry; return true; }
    }
    cache.current=&cache.entries[cache.nextEntry]; cache.nextEntry=(cache.nextEntry+1)&31;
    auto& entry=*cache.current;
    if (entry.displayList) cache.gl->deleteLists(entry.displayList,1);
    entry.displayList=cache.gl->generateLists(1); cache.gl->newList(entry.displayList,0xa3);
    entry.parameters[0]=x0; entry.parameters[1]=x1; entry.parameters[2]=y0; entry.parameters[3]=y1;
    return false;
}
void callCurrentGroundShadow() { groundShadowCache.gl->callList(groundShadowCache.current->displayList); }
void clearGroundShadowCache() {
    auto& cache=groundShadowCache;
    for (auto& entry:cache.entries) {
        cache.current=&entry;
        if (entry.displayList) cache.gl->deleteLists(entry.displayList,1);
        entry.displayList=0;
        for (float& parameter:entry.parameters) parameter=0;
        cache.nextEntry=0;
    }
}
void initializeShapeShadowMask() { shapeShadowMask.data=nullptr; shapeShadowMask.size=0; shapeShadowMask.capacity=0; }
void releaseShapeShadowMask() { if (shapeShadowMask.data) std::free(shapeShadowMask.data); }
void registerShapeShadowMaskCleanup() { std::atexit(releaseShapeShadowMask); }
}
