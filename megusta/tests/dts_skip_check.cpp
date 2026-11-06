#include "../src/dts/shape.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape shape; shape.details.resize(2); shape.details[1].subShape=1; shape.details[1].objectDetail=1;
    shape.firstObjects={0,1}; shape.firstDecals={0,1}; shape.objects.resize(2); shape.decals.resize(2);
    shape.objects[0].startMeshIndex=0; shape.objects[0].numMeshes=2;
    shape.objects[1].startMeshIndex=2; shape.objects[1].numMeshes=2;
    shape.decals[0].startMeshIndex=4; shape.decals[0].numMeshes=2;
    shape.decals[1].startMeshIndex=6; shape.decals[1].numMeshes=2;
    int object=0,decal=0; bool skip=false;
    bool expected[]={true,true,true,false,true,true,true,false};
    for (int mesh=0;mesh<8;++mesh) { assert(shape.checkSkip(mesh,object,decal,1,skip)); assert(skip==expected[mesh]); }
    int rawObject=0; unsigned rawDecal=0;
    for (int mesh=0;mesh<8;++mesh) assert(shape.checkSkip(mesh,&rawObject,&rawDecal,1)==expected[mesh]);
    assert(rawObject==object && rawDecal==static_cast<unsigned>(decal));
    object=decal=0; assert(shape.checkSkip(0,object,decal,0,skip) && !skip);
    assert(!shape.checkSkip(0,object,decal,2,skip));
    shape.assembledMeshes.resize(8); for (auto& mesh:shape.assembledMeshes) mesh.type=DTSMeshStandard;
    shape.smallestVisibleDetail=1; shape.details[0].subShape=0; shape.details[0].objectDetail=0;
    shape.detailFirstSkin={0,2}; shape.detailNumSkins={2,1};
    shape.assembledSkins.resize(3); for (auto& skin:shape.assembledSkins) skin.type=DTSMeshSkin;
    assert(shape.applyDetailSkip(1));
    for (int i=0;i<8;++i) assert((shape.assembledMeshes[i].type==4)==expected[i]);
    assert(shape.details[0].subShape==1 && shape.details[0].objectDetail==1);
    assert(shape.detailFirstSkin[0]==2 && shape.detailNumSkins[0]==1);
    assert(shape.assembledSkins[0].type==4 && shape.assembledSkins[1].type==4 && shape.assembledSkins[2].type==1);
}
