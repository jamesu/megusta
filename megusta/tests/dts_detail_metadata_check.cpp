#include "../src/dts/shape.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape fades; fades.details.resize(5);
    fades.details[0].size=10; fades.details[1].size=9; fades.details[2].size=8;
    fades.details[2].subShape=-1; fades.details[3].size=-1; fades.details[4].size=5;
    fades.initializeDetailAlpha();
    assert(fades.alphaIn[0]==-1 && fades.alphaOut[0]==-1);
    assert(fades.alphaIn[1]==0.15f && fades.alphaOut[1]==0.1f);
    assert(fades.alphaIn[2]==0 && fades.alphaOut[2]==0.1f);
    assert(fades.alphaIn[3]==0 && fades.alphaOut[3]==0);
    assert(fades.alphaIn[4]==0 && fades.alphaOut[4]==0.1f);
    TSShape shape;
    assert(shape.readVersion==-1);
    shape.firstObjects={0}; shape.numObjects={2}; shape.objects.resize(2);
    char materialName[]="mesh";
    shape.materialList.push_back(materialName,0,-1,-1,-1,1,1);
    shape.hasMaterials=true;
    shape.objects[0].startMeshIndex=0; shape.objects[0].numMeshes=1;
    shape.objects[1].startMeshIndex=1; shape.objects[1].numMeshes=1;
    shape.assembledMeshes.resize(2); shape.assembledMeshes[0].type=0; shape.assembledMeshes[1].type=4;
    shape.assembledMeshes[0].base.primitives={{0,6,0},{0,5,0x40000000u}};
    shape.details.resize(2); shape.details[0].subShape=0; shape.details[0].objectDetail=0;
    shape.details[1].subShape=-1; shape.details[1].polyCount=37;
    shape.sequences.resize(2); shape.sequences[0].flags=2; shape.sequences[1].flags=5;
    assert(shape.initializeDetailMetadata());
    assert(shape.details[0].polyCount==5 && shape.details[1].polyCount==37 && shape.shapeFlags==5);
    shape.nodes.resize(1); shape.nodes[0].parentIndex=-1;
    shape.init(); assert(shape.shapeFlags==5 && shape.alphaIn.size()==2 && shape.nodes[0].firstObject==0);
    shape.objects[0].startMeshIndex=2; assert(!shape.initializeDetailMetadata());
}
