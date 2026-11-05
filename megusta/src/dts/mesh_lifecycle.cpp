#include "mesh_lifecycle.h"
#include <vector>

namespace {
const std::uint32_t baseMeshDispatchIdentity=0x4d534842u;
const std::uint32_t skinMeshDispatchIdentity=0x534b494eu;

template<class T> void releaseVector(std::vector<T>& values) {
    std::vector<T>().swap(values);
}
megusta::dts::TSMesh scratchMesh;
megusta::dts::TSDecalMesh scratchDecal;
megusta::dts::TSSkinMesh scratchSkin;
megusta::dts::TSSortedMesh scratchSorted;
}

namespace megusta { namespace dts {
namespace { DisplayListDelete displayListDelete=nullptr; }
void bindDisplayListDelete(DisplayListDelete callback) { displayListDelete=callback; }
void deleteDisplayListIfBound(std::uint32_t list) {
    if (list && displayListDelete) displayListDelete(list,1);
}
void destroyTSMesh(TSMesh* self) {
    self->dispatch=&baseMeshDispatchIdentity;
    deleteDisplayListIfBound(self->displayList);
}
void constructTSSkinMesh(TSSkinMesh* self) {
    self->parentMesh=-1;
    self->displayList=0;
    self->dispatch=&skinMeshDispatchIdentity;
    self->meshType=1;
}
void destroyTSSkinMesh(TSSkinMesh* self) {
    releaseVector(self->skinnedNormals);
    releaseVector(self->skinnedVertices);
    releaseVector(self->boneTransforms);
    destroyTSMesh(self);
}
void destroyTSDecalMesh(TSDecalMesh* self) {
    deleteDisplayListIfBound(self->displayList);
}
} }







extern "C" void DTS_scratchMesh_cleanup(void) {
    megusta::dts::destroyTSMesh(&scratchMesh);
}
extern "C" void DTS_scratchDecal_cleanup(void) {
    megusta::dts::destroyTSDecalMesh(&scratchDecal);
}
extern "C" void DTS_scratchSkin_cleanup(void) {
    megusta::dts::destroyTSSkinMesh(&scratchSkin);
}
extern "C" void DTS_scratchSorted_cleanup(void) {
    megusta::dts::destroyTSMesh(&scratchSorted);
}
