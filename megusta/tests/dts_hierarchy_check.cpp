#include "../src/dts/shape.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape shape;
    shape.nodes.resize(4);
    shape.nodes[0].parentIndex=-1;
    shape.nodes[1].parentIndex=0;
    shape.nodes[2].parentIndex=0;
    shape.nodes[3].parentIndex=1;
    shape.objects.resize(3);
    shape.objects[0].nodeIndex=1; shape.objects[1].nodeIndex=1; shape.objects[2].nodeIndex=0;
    shape.decals.resize(2); shape.decals[0].objectIndex=1; shape.decals[1].objectIndex=1;
    shape.assembledMeshes.resize(2); shape.assembledMeshes[0].type=DTSMeshDecal; shape.assembledMeshes[1].type=DTSMeshDecal;
    shape.assembledMeshes[1].decal.texgenS.resize(1); shape.decals[0].startMeshIndex=0; shape.decals[0].numMeshes=2;
    assert(shape.initializeHierarchy());
    assert(shape.nodes[0].firstChild==1 && shape.nodes[1].nextSibling==2 && shape.nodes[2].nextSibling==-1);
    assert(shape.nodes[1].firstChild==3 && shape.nodes[1].firstObject==0 && shape.objects[0].nextSibling==1);
    assert(shape.nodes[0].firstObject==2 && shape.objects[1].firstDecal==0 && shape.decals[0].nextSibling==1);
    assert(shape.assembledMeshes[0].type==4 && shape.assembledMeshes[1].type==2);
    shape.decals[0].objectIndex=3;
    assert(!shape.initializeHierarchy());
    assert(shape.objects[1].firstDecal==0);

    // 004cc1e0 uses subShapeNumObjects as the direct loop bound, unlike the
    // other object-state paths that add firstObjects. With first=1,count=2,
    // the initializer therefore records 2 and scans object 1 only.
    TSShape materialRanges;
    materialRanges.firstObjects={0,1};
    materialRanges.numObjects={1,2};
    materialRanges.objects.resize(3);
    assert(materialRanges.initializeMaterialList());
    assert(materialRanges.firstTranslucentObjects[0]==1);
    assert(materialRanges.firstTranslucentObjects[1]==2);
}
