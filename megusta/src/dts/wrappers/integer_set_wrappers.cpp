#include "integer_set_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_clearAll_impl(
        megusta::dts::TSIntegerSet* self,std::uint32_t count) {
    self->clearAll(count);
}

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_setAll_impl(
        megusta::dts::TSIntegerSet* self,std::uint32_t count) {
    self->setAll(count);
}

extern "C" std::uint32_t MEGUSTA_DTS_THISCALL TSIntegerSet_testAll_impl(
        const megusta::dts::TSIntegerSet* self,std::uint32_t count) {
    return self->testAll(count) ? 1u : 0u;
}

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_overlap_impl(
        megusta::dts::TSIntegerSet* self,const megusta::dts::TSIntegerSet* other) {
    self->overlap(*other);
}

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_takeAway_impl(
        megusta::dts::TSIntegerSet* self,const megusta::dts::TSIntegerSet* other) {
    self->takeAway(*other);
}

extern "C" int MEGUSTA_DTS_FASTCALL TSIntegerSet_start_impl(
        const megusta::dts::TSIntegerSet* self) {
    return self->start();
}

extern "C" int MEGUSTA_DTS_FASTCALL TSIntegerSet_end_impl(
        const megusta::dts::TSIntegerSet* self) {
    return self->end();
}

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_next_impl(
        const megusta::dts::TSIntegerSet* self,std::uint32_t* index) {
    self->next(index);
}

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_copy_impl(
        megusta::dts::TSIntegerSet* self,const void* other) {
    self->copy(*static_cast<const megusta::dts::TSIntegerSet*>(other));
}

extern "C" megusta::dts::TSIntegerSet* MEGUSTA_DTS_FASTCALL TSIntegerSet_ctor_impl(
        megusta::dts::TSIntegerSet* self) {
    return new (self) megusta::dts::TSIntegerSet();
}

extern "C" megusta::dts::TSIntegerSet* MEGUSTA_DTS_THISCALL TSIntegerSet_copyCtor_impl(
        megusta::dts::TSIntegerSet* self,const megusta::dts::TSIntegerSet* other) {
    return new (self) megusta::dts::TSIntegerSet(*other);
}

extern "C" void MEGUSTA_DTS_THISCALL TSIntegerSet_read_impl(
        megusta::dts::TSIntegerSet* self,megusta::ResourceFile* file) {
    self->read(file);
}
