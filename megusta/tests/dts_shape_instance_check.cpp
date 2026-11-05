#include "../src/dts/shape_instance.h"
#include "../src/dts/shape.h"
#include "../src/dts/texture_manager.h"
#include <cassert>
using namespace megusta::dts;
struct TextureListLifetime {
    TextureListLifetime() { initializeTextureEntryList(); }
    ~TextureListLifetime() { destroyTextureEntryList(); }
};
static int environmentReferences=0,environmentLoads=0;
static int environmentTextureToken;
static void* loadEnvironmentTexture(const char* name,int type,bool clamp) {
    assert(name==nullptr && type==0 && !clamp); ++environmentLoads; return &environmentTextureToken;
}
static void retainEnvironmentTexture(void* entry) {
    assert(entry==&environmentTextureToken); ++environmentReferences;
}
static void* releaseEnvironmentTexture(void* entry) {
    assert(entry==&environmentTextureToken && environmentReferences>0);
    return --environmentReferences ? entry : nullptr;
}
static std::uint32_t environmentTextureName(void* entry) {
    assert(entry==&environmentTextureToken); return 77;
}
int main() {
    TextureListLifetime textureList;
    TSShape shape; shape.smallestVisibleDetail=3; shape.hasMaterials=true;
    shape.nodes.resize(1); shape.objects.resize(1); shape.decals.resize(1);
    shape.nodes[0].parentIndex=-1; shape.defaultRotations.push_back({0,0,0,32767});
    shape.defaultTranslations.push_back({1,2,3});
    shape.objects[0].nodeIndex=0; shape.objects[0].numMeshes=1; shape.objects[0].startMeshIndex=0;
    shape.decals[0].objectIndex=0; shape.decals[0].numMeshes=1; shape.decals[0].startMeshIndex=1;
    shape.firstObjects={0}; shape.numObjects={1};
    shape.firstDecals={0}; shape.numDecals={1};
    shape.detailFirstSkin={0}; shape.detailNumSkins={0};
    shape.assembledMeshes.resize(2); shape.assembledMeshes[0].type=0; shape.assembledMeshes[1].type=2;
    shape.objectStates.push_back({0.75f,2,3}); shape.decalStates.push_back(4);
    shape.firstNodes.push_back(0); shape.numNodes.push_back(1);
    shape.details.push_back({0,0,0,0.0f}); shape.iflMaterials.resize(1);
    char name[]="example.bmp"; shape.materialList.push_back(name,0,1,-1,-1,1,1);
    TSShapeInstance instance(&shape,1);
    assert(instance.instanceLinksReady && instance.meshObjects.size()==1 && instance.decalObjects.size()==1);
    assert(instance.meshObjects[0].visible==0.75f && instance.meshObjects[0].frame==2 && instance.meshObjects[0].materialFrame==3);
    assert(instance.decalObjects[0].frame==4 && shape.assembledMeshes[1].decal.targetMesh==&shape.assembledMeshes[0].base);
    // Constructor's final animateSubtrees pass sees the initial 0x20 dirty
    // bit and initializes each loaded IFL instance at frame zero.
    assert(instance.iflMaterialInstances.size()==1 && instance.iflMaterialInstances[0].frame==0);
    assert(instance.nodeTransforms[0].values[0]==1 && instance.nodeTransforms[0].values[3]==1 && instance.nodeTransforms[0].values[11]==3);
    assert(instance.materialList==&shape.materialList && std::string(shape.materialList.materialNames[0])=="skins/example");
    assert(!instance.alphaIsReflectanceMap);
    shape.assembledMeshes[0].type=1;
    {
        TSShapeInstance skinTargetInstance(&shape,0);
        assert(shape.assembledMeshes[1].decal.targetMesh==&shape.assembledMeshes[0].skin);
    }
    shape.assembledMeshes[0].type=0;
    instance.materialList=new TSMaterialList();
    instance.ownsMaterialList=true;
    instance.setMaterialList(nullptr);
    assert(instance.materialList==nullptr && !instance.ownsMaterialList);
    minimumDetailLevel=2; instance.setCurrentDetail(0,0.25f);
    assert(instance.getCurrentDetail()==2 && instance.currentIntraDetailLevel==1);
    instance.setCurrentDetail(-1,0.5f); assert(instance.getCurrentDetail()==-1 && instance.currentIntraDetailLevel==0.5f);

    // The saved buildInstanceData body marks a detail-map mesh when the map
    // is not -1 and the texture-handle array exists; it does not range-check
    // the map index against the list count.
    TSShape detailShape; detailShape.hasMaterials=true;
    detailShape.objects.resize(1); detailShape.objects[0].numMeshes=1;
    detailShape.objects[0].startMeshIndex=0;
    detailShape.objectStates.push_back({1.0f,0,0});
    detailShape.firstObjects={0}; detailShape.numObjects={1};
    detailShape.details.resize(1); detailShape.details[0].subShape=0;
    detailShape.details[0].objectDetail=0;
    detailShape.detailFirstSkin={0}; detailShape.detailNumSkins={0};
    detailShape.assembledMeshes.resize(1);
    detailShape.assembledMeshes[0].type=0;
    detailShape.assembledMeshes[0].base.primitives={{0,3,0}};
    char detailName[]="detail";
    detailShape.materialList.push_back(detailName,0,-1,-1,7,1,1);
    TSShapeInstance detailInstance(&detailShape,1);
    assert(detailInstance.instanceLinksReady);
    assert((detailShape.assembledMeshes[0].base.meshType&0x40000000u)!=0);

    bindMaterialTextureServices({loadEnvironmentTexture,nullptr,retainEnvironmentTexture,
        releaseEnvironmentTexture,environmentTextureName});
    TSShape environmentShape;
    {
        TSShapeInstance environmentInstance(&environmentShape,0);
        assert(environmentLoads==1 && environmentReferences==1);
        assert(environmentInstance.environmentTextureEntry==&environmentTextureToken);
        assert(environmentInstance.environmentTextureName==77 && environmentInstance.environmentMapEntryPresent);
    }
    assert(environmentReferences==0);
}
