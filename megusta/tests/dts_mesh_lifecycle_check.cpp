#include "../src/dts/mesh_lifecycle.h"
#include "../src/dts/wrappers/mesh_lifecycle_wrappers.h"
#include <cassert>

namespace {
int deletedLists=0;
void deleteLists(std::uint32_t list,int range) {
    assert(list==55 || list==89);
    assert(range==1);
    ++deletedLists;
}
}

int main() {
    using namespace megusta::dts;
    bindDisplayListDelete(deleteLists);
    TSMesh mesh;
    mesh.displayList=55;
    TSMesh_dtor_impl(&mesh);
    assert(mesh.dispatch!=nullptr && deletedLists==1);
    mesh.displayList=0;
    assert(TSMesh_scalarDeletingDtor_impl(&mesh,0)==&mesh);

    TSSkinMesh skin;
    TSSkinMesh_ctor_impl(&skin);
    assert(skin.parentMesh==-1 && skin.displayList==0 && skin.meshType==1);
    skin.boneTransforms.resize(2);
    skin.skinnedVertices.resize(3);
    skin.skinnedNormals.resize(3);
    assert(TSSkinMesh_scalarDeletingDtor_impl(&skin,0)==&skin);
    assert(skin.boneTransforms.empty() && skin.skinnedVertices.empty() && skin.skinnedNormals.empty());

    TSDecalMesh decal;
    decal.displayList=89;
    TSDecalMesh_dtor_impl(&decal);
    assert(deletedLists==2);
    assert(TSDecalMesh_scalarDeletingDtor_impl(&decal,0)==&decal);

    auto* heapMesh=new TSMesh;
    assert(TSMesh_scalarDeletingDtor_impl(heapMesh,1)==heapMesh);
    auto* heapSkin=new TSSkinMesh;
    TSSkinMesh_ctor_impl(heapSkin);
    assert(TSSkinMesh_scalarDeletingDtor_impl(heapSkin,1)==heapSkin);
    auto* heapDecal=new TSDecalMesh;
    assert(TSDecalMesh_scalarDeletingDtor_impl(heapDecal,1)==heapDecal);
    bindDisplayListDelete(nullptr);
}
