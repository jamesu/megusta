#include "../src/dts/shape.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape shape; TSShapeSequence seq; seq.numKeyframes=2;
    // Explicit buffer indices are independent of the shape's node prefix.
    shape.nodes.resize(3);
    seq.rotationMatters.bits[0]=3; seq.frameMatters.bits[0]=3; seq.decalMatters.bits[0]=3;
    legacyKeyframeScratch={{1,1,1}};
    int rotation[]={99,1,2,3,4},translation[]={99,5,6,7,8};
    int object[]={99,9,10,11,12},decal[]={99,13,14,15,16};
    shape.rearrangeKeyframeData(&seq,0,reinterpret_cast<unsigned char*>(rotation),reinterpret_cast<unsigned char*>(translation),
        reinterpret_cast<unsigned char*>(object),reinterpret_cast<unsigned char*>(decal),4,4,4,4);
    assert(seq.baseRotation==1 && seq.baseTranslation==1 && seq.baseObjectState==1 && seq.baseDecalState==1);
    assert(rotation[1]==1 && rotation[2]==3 && rotation[3]==2 && rotation[4]==4);
    assert(translation[2]==7 && object[2]==11 && decal[2]==15);
}
