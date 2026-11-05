#include "material_list_wrappers.h"
using namespace megusta::dts;

extern "C" megusta::dts::TSMaterialList* MEGUSTA_DTS_FASTCALL
TSMaterialList_ctor_impl(megusta::dts::TSMaterialList* self) {
    return self ? new(self) megusta::dts::TSMaterialList() : nullptr;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSMaterialList_dtor_impl(
    megusta::dts::TSMaterialList* self) {
    if (self) self->~TSMaterialList();
}

extern "C" void* MEGUSTA_DTS_THISCALL TSMaterialList_scalarDeletingDtor_impl(
    megusta::dts::TSMaterialList* self,std::uint8_t deleteObject) {
    TSMaterialList_dtor_impl(self);
    if (self && (deleteObject&1u)) ::operator delete(self);
    return self;
}

extern "C" void MEGUSTA_DTS_FASTCALL TSMaterialList_free_impl(
    megusta::dts::TSMaterialList* self) { if (self) self->free(); }

extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_remap_impl(
    megusta::dts::TSMaterialList* self,int destination,int source) {
    self->remap(destination,source);
}

extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_allocate_impl(
    megusta::dts::TSMaterialList* self,std::uint32_t count) { self->allocate(count); }

extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_load_impl(
    megusta::dts::TSMaterialList* self,std::uint32_t index) { self->load(index); }

extern "C" void MEGUSTA_DTS_FASTCALL DTS_MaterialList_ctor_impl(
    megusta::dts::DTSMaterialList* self) {
    if (self) new(self) megusta::dts::DTSMaterialList();
}

extern "C" void MEGUSTA_DTS_FASTCALL DTS_MaterialList_dtor_impl(
    megusta::dts::DTSMaterialList* self) {
    if (self) self->~DTSMaterialList();
}

extern "C" void* MEGUSTA_DTS_THISCALL DTS_MaterialList_scalarDeletingDtor_impl(
    megusta::dts::DTSMaterialList* self,std::uint8_t deleteObject) {
    DTS_MaterialList_dtor_impl(self);
    if (self && (deleteObject&1u)) ::operator delete(self);
    return self;
}

extern "C" void MEGUSTA_DTS_THISCALL DTS_MaterialList_load_index_impl(
    megusta::dts::DTSMaterialList* self,std::uint32_t index) { self->loadIndex(index); }

extern "C" int MEGUSTA_DTS_FASTCALL DTS_MaterialList_load_all_impl(
    megusta::dts::DTSMaterialList* self) { return self->loadAll() ? 1 : 0; }

extern "C" void MEGUSTA_DTS_THISCALL DTS_MaterialList_unload_impl(
    megusta::dts::DTSMaterialList* self,std::uint32_t index) { self->unload(index); }

extern "C" void MEGUSTA_DTS_FASTCALL DTS_MaterialList_free_impl(
    megusta::dts::DTSMaterialList* self) { if (self) self->free(); }

extern "C" void MEGUSTA_DTS_FASTCALL TSMaterialList_tsmlTransform_impl(
    megusta::dts::TSMaterialList* self) { self->tsmlTransform(); }

extern "C" void MEGUSTA_DTS_THISCALL TSMaterialList_push_back_impl(
    megusta::dts::TSMaterialList* self,char* name,std::uint32_t flags,
    int reflectanceMap,int bumpMap,int detailMap,float detailScale,float reflectionAmount) {
    self->push_back(name,flags,reflectanceMap,bumpMap,detailMap,detailScale,reflectionAmount);
}

extern "C" int MEGUSTA_DTS_THISCALL DTS_MaterialList_push_back_impl(
    megusta::dts::DTSMaterialList* self,char* name) {
    return self->push_back(name);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSMaterialList_read_impl(
    megusta::dts::TSMaterialList* self,megusta::ResourceFile* file) {
    return self->read(file);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL DTS_MaterialList_read_impl(
    megusta::dts::DTSMaterialList* self,megusta::ResourceFile* file) {
    return self->read(file);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL DTS_MaterialList_readText_impl(
    megusta::dts::DTSMaterialList* self,megusta::ResourceFile* file,char first) {
    return self->readText(file,first);
}
