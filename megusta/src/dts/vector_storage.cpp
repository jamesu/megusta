#include "vector_storage.h"
#include <cstdlib>

namespace megusta { namespace dts {

extern "C" bool MEGUSTA_DTS_CDECL DTS_Vector_resizeStorage(
        std::uint32_t* capacity,std::uint32_t* count,void** data,
        std::uint32_t requestedCount,int elementSize) {
    if (requestedCount) {
        std::uint32_t blocks=requestedCount>>4;
        if (requestedCount&0x0fu) ++blocks;
        const std::uint32_t bytes=blocks*static_cast<std::uint32_t>(elementSize)*16u;
        void* resized=*data ? std::realloc(*data,bytes) : std::malloc(bytes);
        *data=resized;
        *count=requestedCount;
        *capacity=blocks<<4;
        return true;
    }
    if (*data) std::free(*data);
    *data=nullptr;
    *capacity=0;
    *count=0;
    return true;
}

bool DTSVectorStorage::resize(std::uint32_t requestedCount,std::uint32_t elementSize) {
    return DTS_Vector_resizeStorage(&capacity,&count,&data,requestedCount,static_cast<int>(elementSize));
}

bool DTSVectorStorage::increment(std::uint32_t amount,std::uint32_t elementSize) {
    count+=amount;
    if (capacity<count) resize(count,elementSize);
    return true;
}

void DTSVectorStorage::construct(std::uint32_t initialCount,std::uint32_t elementSize) {
    count=0; data=nullptr; capacity=0;
    if (initialCount && resize(initialCount,elementSize)) count=0;
}

extern "C" DTSVectorStorage* MEGUSTA_DTS_THISCALL DTS_Vector64_ctor(DTSVectorStorage* self,std::uint32_t initialCount) {
    self->construct(initialCount,0x40); return self;
}
extern "C" DTSVectorStorage* MEGUSTA_DTS_THISCALL DTS_Vector12_ctor(DTSVectorStorage* self,std::uint32_t initialCount) {
    self->construct(initialCount,0x0c); return self;
}
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector64_setSize(DTSVectorStorage* self,std::uint32_t count) { self->resize(count,0x40); }
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector12_setSize(DTSVectorStorage* self,std::uint32_t count) { self->resize(count,0x0c); }
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector4_setSize(DTSVectorStorage* self,std::uint32_t count) { self->resize(count,4); }
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector8_setSize(DTSVectorStorage* self,std::uint32_t count) { self->resize(count,8); }
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector4_incrementSize(DTSVectorStorage* self,int amount) {
    self->increment(static_cast<std::uint32_t>(amount),4);
}

} }
