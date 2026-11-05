#include "putt_map_runtime.h"
#include "scene_object.h"
#include "random.h"
#include "recovered_binding_primitives.h"
#include "../engine/string_services.h"
#include <cmath>
namespace megusta {
void estimatePutt(const float* position,const float* target,float* heading,float* power,bool randomized,
                  const PuttEstimateServices& s) {
    if (!heading || !power) { s.fatal(s.context); return; }
    float nearest[3]={};
    if (!position) position=s.currentBallPosition(s.context);
    if (!target) { findNearestCup(s.objects,position,nearest,nullptr,false,s.fatal,s.context); target=nearest; }
    float x=position[0]-target[0],y=position[1]-target[1],z=position[2]-target[2];
    *heading=0; *power=0.1f;
    if (x==0 && y==0) return;
    float z2=z*z;
    float distance=binding_math_sqrtf(x*x+y*y+z2);
    if (z>0) distance=static_cast<float>(static_cast<long double>(distance)+binding_math_sqrtf(z2));
    else if (z< -1) distance=static_cast<float>(static_cast<long double>(distance)-binding_math_sqrtf(z2));
    if (distance<1) distance=1;
    const float root=binding_math_sqrtf(distance);
    const float divisor=binding_math_sqrtf(350.0f);
    *power=static_cast<float>(static_cast<long double>(root)/divisor);
    if (randomized) *power+=static_cast<float>(static_cast<int>(s.random.generate()&0x1ffu)-256)*0.0005f;
    float adjusted=static_cast<float>(*power*1.05);
    if (adjusted>=0.9f) adjusted=0.9f; else if (adjusted<=0.1f) adjusted=0.1f;
    *power=adjusted;
    // The saved body takes asin after the horizontal magnitude and adjusts
    // the quadrant from the sign of X. The x87 argument is carried in ST(0),
    // so the decompiler omits the normalized Y operand.
    const float horizontal=binding_math_sqrtf(x*x+y*y);
    long double angle=std::asin(static_cast<long double>(y)/horizontal)*57.29578L;
    if (x<0.0f) angle=180.0L-angle;
    if (randomized) angle=(static_cast<long double>(s.random.generate()&255u)*0.0078125L-1.0L)+angle;
    *heading=static_cast<float>(angle);
}
long double findNearestCup(SceneObjectCollection& objects,const float* position,float* outputPosition,
                           int* outputIndex,bool horizontalOnly,void (*fatal)(void*),void* context) {
    if (!position) { fatal(context); return 100000.0L; }
    if (outputPosition) outputPosition[0]=outputPosition[1]=outputPosition[2]=0;
    if (outputIndex) *outputIndex=0;
    float best=100000.0f;
    for (int i=1;i<objects.objectCount;++i) {
        SceneObjectRecord& object=objects.objects[i];
        if (!strings::findSubstring(object.name,"cup_0")) continue;
        const float* cup=object.getPosition();
        float x=position[0]-cup[0],y=position[1]-cup[1],z=position[2]-cup[2];
        float squared=horizontalOnly ? x*x+y*y : x*x+z*z+y*y;
        long double distance=std::sqrt(static_cast<long double>(squared));
        if (distance<best) {
            best=static_cast<float>(distance);
            if (outputPosition) { outputPosition[0]=cup[0]; outputPosition[1]=cup[1]; outputPosition[2]=cup[2]; }
            if (outputIndex) *outputIndex=i;
        }
    }
    return static_cast<long double>(best);
}
}
