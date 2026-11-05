#include "shape_alloc.h"
#include <cstring>
#include <new>
namespace megusta { namespace dts {
void TSShapeAlloc::setRead(const void* a,const void* b,const void* c,char sizing) {
    buffer32=static_cast<const std::uint8_t*>(a); buffer16=static_cast<const std::uint8_t*>(b); buffer8=static_cast<const std::uint8_t*>(c);
    guard32=savedGuard32=0; guard16=savedGuard16=0; guard8=savedGuard8=0;
    guardMismatch=false;
    if (sizing) { destination=nullptr; size=0; }
    multiplier=1; mode=0;
}
void TSShapeAlloc::doAlloc() { destination=static_cast<std::uint8_t*>(::operator new(size)); size=0; }
void TSShapeAlloc::align32() { allocShape8(((size+3u)&~3u)-size); }
void TSShapeAlloc::align128() { allocShape8(((size+15u)&~15u)-size); }
std::uint32_t TSShapeAlloc::get32() { std::uint32_t x; std::memcpy(&x,buffer32,4); buffer32+=4; return x; }
std::uint16_t TSShapeAlloc::get16() { std::uint16_t x; std::memcpy(&x,buffer16,2); buffer16+=2; return x; }
std::uint8_t TSShapeAlloc::get8() { return *buffer8++; }
void TSShapeAlloc::get32Block(void* out,int count) { std::memcpy(out,buffer32,count*4); buffer32+=count*4; }
void TSShapeAlloc::get16Block(void* out,int count) { std::memcpy(out,buffer16,count*2); buffer16+=count*2; }
void TSShapeAlloc::allocate(int count,unsigned width) { auto bytes=multiplier*count*width; if (destination) destination+=bytes; size+=bytes; }
void* TSShapeAlloc::allocShape32(int count) { void* result=destination; allocate(count,4); return result; }
void* TSShapeAlloc::allocShape16(int count) { void* result=destination; allocate(count,2); return result; }
void* TSShapeAlloc::allocShape8(int count) { void* result=destination; allocate(count,1); return result; }
void* TSShapeAlloc::allocShapePointers(int count) {
    if (count) {
        auto alignment=static_cast<std::uint32_t>(alignof(void*));
        allocShape8((alignment-size%alignment)%alignment);
    }
    return allocShape8(count*static_cast<int>(sizeof(void*)));
}
const void* TSShapeAlloc::getPointer32(int count) { auto* result=buffer32; buffer32+=count*4; return result; }
const void* TSShapeAlloc::getPointer16(int count) { auto* result=buffer16; buffer16+=count*2; return result; }
const void* TSShapeAlloc::getPointer8(int count) { auto* result=buffer8; buffer8+=count; return result; }
void* TSShapeAlloc::copy(const std::uint8_t*& source,int count,unsigned width,bool returnSource) {
    auto* result=destination ? destination : (returnSource ? const_cast<std::uint8_t*>(source) : nullptr);
    auto bytes=multiplier*count*width;
    if (destination) { std::memcpy(destination,source,bytes); destination+=bytes; }
    source+=count*width; size+=bytes; return result;
}
void* TSShapeAlloc::copyToShape32(int n,char source) { return copy(buffer32,n,4,source!=0); }
void* TSShapeAlloc::copyToShape16(int n,char source) { return copy(buffer16,n,2,source!=0); }
void* TSShapeAlloc::copyToShape8(int n,char source) { return copy(buffer8,n,1,source!=0); }
bool TSShapeAlloc::checkGuard32() { savedGuard32=get32(); bool ok=savedGuard32==guard32++; guardMismatch|=!ok; return ok; }
bool TSShapeAlloc::checkGuard16() { savedGuard16=get16(); bool ok=savedGuard16==guard16++; guardMismatch|=!ok; return ok; }
bool TSShapeAlloc::checkGuard8() { savedGuard8=get8(); bool ok=savedGuard8==guard8++; guardMismatch|=!ok; return ok; }
void TSShapeAlloc::checkGuard() { checkGuard32(); checkGuard16(); checkGuard8(); }
} }

