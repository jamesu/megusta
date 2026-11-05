#pragma once
#include <cstdint>
#include "vector_storage.h"
namespace megusta { class ResourceFile; namespace dts {
// Normalized owner for one recovered legacy word/byte stream. Growth and
// count reset follow the original allocator; ownership is per shape rather
// than the executable's process-global buffers.
template<class T> class DTSLegacyStream {
    T* elements_=nullptr;
    std::uint32_t count_=0,capacity_=0;
public:
    DTSLegacyStream()=default;
    ~DTSLegacyStream();
    DTSLegacyStream(const DTSLegacyStream&)=delete;
    DTSLegacyStream& operator=(const DTSLegacyStream&)=delete;
    DTSLegacyStream(DTSLegacyStream&& other) noexcept;
    DTSLegacyStream& operator=(DTSLegacyStream&& other) noexcept;
    void init();
    void adoptExternal(T** allocation) { elements_=*allocation; }
    void publishExternal(T** allocation) const { *allocation=elements_; }
    T* allocate(int count);
    T* allocateExternal(T** allocation,int count);
    std::uint32_t size() const { return count_; }
    T* data() { return elements_; }
    const T* data() const { return elements_; }
    T& operator[](std::uint32_t index) { return elements_[index]; }
    const T& operator[](std::uint32_t index) const { return elements_[index]; }
};

class DTSLegacyAlloc {
public:
    DTSLegacyStream<std::uint32_t> words32;
    DTSLegacyStream<std::uint16_t> words16;
    DTSLegacyStream<std::uint8_t> bytes8;
    std::uint32_t guard32=0;
    std::uint16_t guard16=0;
    std::uint8_t guard8=0;
    void init32(); void init16(); void init8();
    std::uint32_t dwordCount32() const;
    std::uint32_t dwordCount16() const;
    std::uint32_t dwordCount8() const;
    std::uint32_t offset16() const;
    std::uint32_t* alloc32(int count);
    std::uint16_t* alloc16(int count);
    std::uint8_t* alloc8(int count);
    void guard();
    std::uint32_t readAppend32(ResourceFile& file);
    std::uint32_t readStore32(ResourceFile& file,std::uint32_t& value);
    void read32(ResourceFile& file,int count);
    void read16(ResourceFile& file,int count);
    void read8(ResourceFile& file,int count);
};
} }

megusta::dts::DTSLegacyAlloc& DTSLegacy_addressAllocator(void);

// Address-level process-global legacy reader entry points recovered from the
// executable. The pointer arguments remain caller-owned, as in the original.

