#include "../src/dts/shape.h"
#include "../src/engine/resource_file.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape shape; shape.readVersion=16; shape.nodes.resize(2); shape.nodeStates.resize(5);
    for (int i=0;i<5;++i) { shape.nodeStates[i].rotation[0]=static_cast<short>(i); shape.nodeStates[i].translation[0]=static_cast<float>(i); }
    shape.sequences.resize(2);
    shape.sequences[0].baseRotation=2; shape.sequences[0].baseTranslation=2;
    shape.sequences[0].firstGroundFrame=3; shape.sequences[0].numGroundFrames=2;
    shape.sequences[1].baseRotation=4; shape.sequences[1].baseTranslation=4;
    assert(shape.normalizeLegacyPoses());
    assert(shape.defaultRotations.size()==2 && shape.nodeRotations.size()==3 && shape.nodeRotations[0].x==2);
    assert(shape.defaultTranslations[1].x==1 && shape.nodeTranslations[2].x==4);
    assert(shape.groundRotations.size()==2 && shape.groundRotations[0].x==3 && shape.groundTranslations[1].x==4);
    assert(shape.sequences[0].baseRotation==0 && shape.sequences[0].firstGroundFrame==0);
    assert(shape.sequences[1].baseTranslation==2 && shape.sequences[1].firstGroundFrame==2);
    shape.sequences[0].firstGroundFrame=1; assert(!shape.normalizeLegacyPoses());
    TSShape oldSlots; oldSlots.readVersion=15;
    oldSlots.legacyPre16Words={0xffffffffu,7};
    const std::uint32_t oldTail[]={1,4,0,0,0};
    megusta::ResourceFile oldFile(oldTail,sizeof(oldTail));
    assert(oldSlots.readLegacyTail(oldFile,""));
    assert(oldSlots.meshes.size()==2 && oldSlots.meshes[0].type==4 && oldSlots.meshes[1].type==4);
}
