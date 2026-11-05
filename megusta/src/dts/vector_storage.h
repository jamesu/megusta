#pragma once
#include <cstdint>

namespace megusta { namespace dts {

#if defined(_MSC_VER) && !defined(__clang__)
// These C-linkage adapters take the object as an explicit first argument.
// MSVC only accepts __thiscall on native member functions, so use its default
// free-function convention here; declarations and definitions share it.
// ClangCL accepts the GNU thiscall attribute and keeps the x86 register ABI.
#define MEGUSTA_DTS_THISCALL
#define MEGUSTA_DTS_CDECL __cdecl
#define MEGUSTA_DTS_FASTCALL __fastcall
#else
#define MEGUSTA_DTS_THISCALL __attribute__((thiscall))
#define MEGUSTA_DTS_CDECL __attribute__((cdecl))
#define MEGUSTA_DTS_FASTCALL __attribute__((fastcall))
#endif

// Recovered 12-byte DTS vector header: count, data pointer, capacity. The
// original allocates capacity in blocks of sixteen elements.
struct DTSVectorStorage {
    std::uint32_t count=0;
    void* data=nullptr;
    std::uint32_t capacity=0;

    bool resize(std::uint32_t requestedCount,std::uint32_t elementSize);
    bool increment(std::uint32_t amount,std::uint32_t elementSize);
    void construct(std::uint32_t initialCount,std::uint32_t elementSize);
};
static_assert(sizeof(DTSVectorStorage)==12,"DTS vector header width on the 32-bit target");

extern "C" bool MEGUSTA_DTS_CDECL DTS_Vector_resizeStorage(
    std::uint32_t* capacity,std::uint32_t* count,void** data,
    std::uint32_t requestedCount,int elementSize);
extern "C" DTSVectorStorage* MEGUSTA_DTS_THISCALL DTS_Vector64_ctor(DTSVectorStorage*,std::uint32_t);
extern "C" DTSVectorStorage* MEGUSTA_DTS_THISCALL DTS_Vector12_ctor(DTSVectorStorage*,std::uint32_t);
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector64_setSize(DTSVectorStorage*,std::uint32_t);
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector12_setSize(DTSVectorStorage*,std::uint32_t);
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector4_setSize(DTSVectorStorage*,std::uint32_t);
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector8_setSize(DTSVectorStorage*,std::uint32_t);
extern "C" void MEGUSTA_DTS_THISCALL DTS_Vector4_incrementSize(DTSVectorStorage*,int);

} }
