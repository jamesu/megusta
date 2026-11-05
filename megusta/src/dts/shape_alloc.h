#pragma once
#include <cstddef>
#include <cstdint>
#include "vector_storage.h"
namespace megusta { namespace dts {
class TSShapeAlloc {
public:
    const std::uint8_t *buffer32=nullptr,*buffer16=nullptr,*buffer8=nullptr;
    // Original member ordering: all read guards precede all saved guards.
    std::uint32_t guard32=0;
    std::uint16_t guard16=0;
    std::uint8_t guard8=0;
    std::uint32_t savedGuard32=0;
    std::uint16_t savedGuard16=0;
    std::uint8_t savedGuard8=0;
    bool guardMismatch=false;
    std::uint8_t* destination=nullptr;
    std::uint32_t size=0,multiplier=1,mode=0;
    void setRead(const void* source32,const void* source16,const void* source8,char sizing);
    void doAlloc();
    void align32();
    void align128();
    std::uint32_t get32();
    std::uint16_t get16();
    std::uint8_t get8();
    void get32Block(void* out,int count);
    void get16Block(void* out,int count);
    void* allocShape32(int count);
    void* allocShape16(int count);
    void* allocShape8(int count);
    // Runtime pointer slots are native-width; DTS stream words remain 32-bit.
    void* allocShapePointers(int count);
    const void* getPointer32(int count);
    const void* getPointer16(int count);
    const void* getPointer8(int count);
    void* copyToShape32(int count,char returnSource);
    void* copyToShape16(int count,char returnSource);
    void* copyToShape8(int count,char returnSource);
    bool checkGuard32();
    bool checkGuard16();
    bool checkGuard8();
    void checkGuard();
private:
    void allocate(int count,unsigned width);
    void* copy(const std::uint8_t*& source,int count,unsigned width,bool returnSource);
};
} }

static_assert(sizeof(void*)!=4 ||
              (offsetof(megusta::dts::TSShapeAlloc,guard32)==0x0c &&
               offsetof(megusta::dts::TSShapeAlloc,guard16)==0x10 &&
               offsetof(megusta::dts::TSShapeAlloc,guard8)==0x12 &&
               offsetof(megusta::dts::TSShapeAlloc,savedGuard32)==0x14 &&
               offsetof(megusta::dts::TSShapeAlloc,savedGuard16)==0x18 &&
               offsetof(megusta::dts::TSShapeAlloc,savedGuard8)==0x1a &&
               offsetof(megusta::dts::TSShapeAlloc,destination)==0x1c &&
               offsetof(megusta::dts::TSShapeAlloc,size)==0x20 &&
               offsetof(megusta::dts::TSShapeAlloc,multiplier)==0x24 &&
               offsetof(megusta::dts::TSShapeAlloc,mode)==0x28 &&
               sizeof(megusta::dts::TSShapeAlloc)==0x2c),
              "TSShapeAlloc Win32 member offsets must match the recovered object");

// These decompiler prototypes are void, but recovered callers consume EAX as
// the pre-advance destination pointer, so the address adapters preserve it.
