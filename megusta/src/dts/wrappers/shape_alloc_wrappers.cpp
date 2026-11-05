#include "shape_alloc_wrappers.h"
using namespace megusta::dts;

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_allocShape32_impl(
        megusta::dts::TSShapeAlloc* self,int count) {
    return self->allocShape32(count);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_getPointer32_impl(
        megusta::dts::TSShapeAlloc* self,int count) {
    return const_cast<void*>(self->getPointer32(count));
}

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_allocShape16_impl(
        megusta::dts::TSShapeAlloc* self,int count) {
    return self->allocShape16(count);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_getPointer16_impl(
        megusta::dts::TSShapeAlloc* self,int count) {
    return const_cast<void*>(self->getPointer16(count));
}

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_allocShape8_impl(
        megusta::dts::TSShapeAlloc* self,int count) {
    return self->allocShape8(count);
}

extern "C" void* MEGUSTA_DTS_THISCALL TSShapeAlloc_getPointer8_impl(
        megusta::dts::TSShapeAlloc* self,int count) {
    return const_cast<void*>(self->getPointer8(count));
}
