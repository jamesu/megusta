#include "legacy_alloc.h"
#include "../engine/resource_file.h"
#include <cstring>
#include <new>
namespace megusta { namespace dts {
template<class T> DTSLegacyStream<T>::~DTSLegacyStream() {
    ::operator delete(elements_);
}
template<class T> DTSLegacyStream<T>::DTSLegacyStream(DTSLegacyStream&& other) noexcept
    : elements_(other.elements_),count_(other.count_),capacity_(other.capacity_) {
    other.elements_=nullptr; other.count_=0; other.capacity_=0;
}
template<class T> DTSLegacyStream<T>& DTSLegacyStream<T>::operator=(DTSLegacyStream&& other) noexcept {
    if (this==&other) return *this;
    ::operator delete(elements_);
    elements_=other.elements_; count_=other.count_; capacity_=other.capacity_;
    other.elements_=nullptr; other.count_=0; other.capacity_=0;
    return *this;
}
template<class T> void DTSLegacyStream<T>::init() {
    // Original init resets the logical count and remembered capacity but leaves
    // the old allocation attached until the next allocation replaces it.
    count_=0; capacity_=0;
}
template<class T> T* DTSLegacyStream<T>::allocate(int amount) {
    const std::int32_t required=count_+amount;
    if (capacity_<required) {
        const std::int32_t blocks=required/25000+1;
        const std::int32_t newCapacity=blocks*25000;
        T* replacement=static_cast<T*>(::operator new(static_cast<std::size_t>(newCapacity)*sizeof(T)));
        if (elements_)
            std::memcpy(replacement,elements_,static_cast<std::size_t>(count_)*sizeof(T));
        ::operator delete(elements_);
        elements_=replacement;
        capacity_=newCapacity;
    }
    T* result=elements_ ? elements_+count_ : elements_;
    count_=required;
    return result;
}
template<class T> T* DTSLegacyStream<T>::allocateExternal(T** allocation,int amount) {
    // The original helpers receive the address of the stream's pointer and
    // update it only when the 25,000-element capacity boundary is crossed.
    if (elements_!=*allocation) elements_=*allocation;
    const std::int32_t required=count_+amount;
    if (capacity_<required) {
        const std::int32_t blocks=required/25000+1;
        const std::int32_t newCapacity=blocks*25000;
        T* replacement=static_cast<T*>(::operator new(
            static_cast<std::size_t>(newCapacity)*sizeof(T)));
        if (elements_)
            std::memcpy(replacement,elements_,static_cast<std::size_t>(count_)*sizeof(T));
        ::operator delete(elements_);
        elements_=replacement;
        capacity_=newCapacity;
        *allocation=elements_;
    }
    T* result=elements_ ? elements_+count_ : elements_;
    count_=required;
    return result;
}
template class DTSLegacyStream<std::uint32_t>;
template class DTSLegacyStream<std::uint16_t>;
template class DTSLegacyStream<std::uint8_t>;
void DTSLegacyAlloc::init32() { words32.init(); guard32=0; }
void DTSLegacyAlloc::init16() { words16.init(); guard16=0; }
void DTSLegacyAlloc::init8() { bytes8.init(); guard8=0; }
std::uint32_t DTSLegacyAlloc::dwordCount32() const { return static_cast<std::uint32_t>(words32.size()); }
std::uint32_t DTSLegacyAlloc::dwordCount16() const { auto n=static_cast<std::uint32_t>(words16.size()); return static_cast<std::int32_t>(n+(n&1u ? 2u : 0u))>>1; }
std::uint32_t DTSLegacyAlloc::dwordCount8() const { auto n=static_cast<std::uint32_t>(bytes8.size()); return static_cast<std::int32_t>(n+(n&3u ? 4u : 0u))>>2; }
std::uint32_t DTSLegacyAlloc::offset16() const { return static_cast<std::uint32_t>(words16.size()); }
std::uint32_t* DTSLegacyAlloc::alloc32(int n) { return words32.allocate(n); }
std::uint16_t* DTSLegacyAlloc::alloc16(int n) { return words16.allocate(n); }
std::uint8_t* DTSLegacyAlloc::alloc8(int n) { return bytes8.allocate(n); }
void DTSLegacyAlloc::guard() { *alloc32(1)=guard32++; *alloc16(1)=guard16++; *alloc8(1)=guard8++; }
std::uint32_t DTSLegacyAlloc::readAppend32(ResourceFile& file) { std::uint32_t value=0; file.read(&value,4,1); *alloc32(1)=value; return value; }
std::uint32_t DTSLegacyAlloc::readStore32(ResourceFile& file,std::uint32_t& value) { file.read(&value,4,1); return value; }
void DTSLegacyAlloc::read32(ResourceFile& file,int n) { auto* p=alloc32(n); file.read(p,4,n); }
void DTSLegacyAlloc::read16(ResourceFile& file,int n) { auto* p=alloc16(n); file.read(p,2,n); }
void DTSLegacyAlloc::read8(ResourceFile& file,int n) { auto* p=alloc8(n); file.read(p,1,n); }
} }

namespace {
megusta::dts::DTSLegacyAlloc addressLegacyAlloc;
}

megusta::dts::DTSLegacyAlloc& DTSLegacy_addressAllocator(void) {
    return addressLegacyAlloc;
}

