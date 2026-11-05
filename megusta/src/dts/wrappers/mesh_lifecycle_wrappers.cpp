#include "mesh_lifecycle_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_FASTCALL TSMesh_dtor_impl(megusta::dts::TSMesh* self) {
    megusta::dts::destroyTSMesh(self);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSMesh_scalarDeletingDtor_impl(
        megusta::dts::TSMesh* self,std::uint8_t deleteObject) {
    TSMesh_dtor_impl(self);
    if (deleteObject&1u) delete self;
    return self;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSSkinMesh_ctor_impl(megusta::dts::TSSkinMesh* self) {
    megusta::dts::constructTSSkinMesh(self);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSSkinMesh_scalarDeletingDtor_impl(
        megusta::dts::TSSkinMesh* self,std::uint8_t deleteObject) {
    megusta::dts::destroyTSSkinMesh(self);
    if (deleteObject&1u) delete self;
    return self;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSDecalMesh_dtor_impl(megusta::dts::TSDecalMesh* self) {
    megusta::dts::destroyTSDecalMesh(self);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSDecalMesh_scalarDeletingDtor_impl(
        megusta::dts::TSDecalMesh* self,std::uint8_t deleteObject) {
    TSDecalMesh_dtor_impl(self);
    if (deleteObject&1u) delete self;
    return self;
}
