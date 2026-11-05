#include "../src/dts/thread.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShapeSequence seq; seq.numKeyframes=5; int first=-1,second=-1; float fraction=-1;
    selectKeyframes(0.625f,&seq,&first,&second,&fraction);
    assert(first==2 && second==3 && fraction==0.5f);
    selectKeyframes(1.0f,&seq,&first,&second,&fraction);
    assert(first==4 && second==4 && fraction==0);
    seq.flags=0x10;
    selectKeyframes(0.9f,&seq,&first,&second,&fraction);
    assert(first==4 && second==0 && fraction>0.49f && fraction<0.51f);
    selectKeyframes(0.0f,&seq,nullptr,nullptr,&fraction); assert(fraction==0);
}
