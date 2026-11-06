#include "../src/dts/shape.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape shape; shape.firstObjects={0,2}; shape.numObjects={2,3};
    shape.objects.resize(3); shape.assembledMeshes.resize(3);
    for (int i=0;i<3;++i) {
        shape.objects[i].startMeshIndex=i; shape.objects[i].numMeshes=1;
        shape.assembledMeshes[i].type=DTSMeshStandard;
    }
    shape.hasMaterials=true; char materialA[]="a",materialB[]="b";
    shape.materialList.push_back(materialA,0,-1,-1,-1,1,1);
    shape.materialList.push_back(materialB,0,-1,-1,-1,1,1);
    shape.materialList.setFlags(0,0);
    shape.materialList.setFlags(1,4);
    shape.assembledMeshes[0].base.primitives={{0,3,0},{0,3,0x10000001u}};
    shape.assembledMeshes[1].base.primitives={{0,3,1}};
    shape.assembledMeshes[2].base.primitives={{0,3,1}};
    assert(shape.initializeMaterialList());
    assert((shape.shapeFlags&0x80u)!=0 && shape.firstTranslucentObjects[0]==1 && shape.firstTranslucentObjects[1]==0);
    shape.shapeFlags=0; shape.firstTranslucentObjects.clear();
    shape.materialList.setFlags(1,0x20000004u);
    assert(shape.initializeMaterialList());
    assert(!(shape.shapeFlags&0x80u) && shape.firstTranslucentObjects[0]==2 && shape.firstTranslucentObjects[1]==3);
    shape.assembledMeshes[0].base.primitives[0].material=0x10000001u;
    assert(shape.initializeMaterialList());
    assert(!(shape.shapeFlags&0x80u) && shape.firstTranslucentObjects[0]==2);
}
