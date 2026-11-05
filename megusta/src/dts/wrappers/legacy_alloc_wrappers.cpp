#include "legacy_alloc_wrappers.h"
using namespace megusta::dts;

#define addressLegacyAlloc DTSLegacy_addressAllocator()

extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_getDWordCount32_impl(void) {
    return addressLegacyAlloc.dwordCount32();
}

extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_getDWordCount16_impl(void) {
    return addressLegacyAlloc.dwordCount16();
}

extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_getDWordCount8_impl(void) {
    return addressLegacyAlloc.dwordCount8();
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldInitAlloc32_impl(void) {
    addressLegacyAlloc.init32();
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldInitAlloc16_impl(void) {
    addressLegacyAlloc.init16();
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldInitAlloc8_impl(void) {
    addressLegacyAlloc.init8();
}

extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_oldAllocOffset16_impl(void) {
    return addressLegacyAlloc.offset16();
}

extern "C" std::uint32_t* MEGUSTA_DTS_CDECL DTSLegacy_oldAlloc32_impl(
        std::uint32_t** allocation,int count) {
    return addressLegacyAlloc.words32.allocateExternal(allocation,count);
}

extern "C" std::uint16_t* MEGUSTA_DTS_CDECL DTSLegacy_oldAlloc16_impl(
        std::uint16_t** allocation,int count) {
    return addressLegacyAlloc.words16.allocateExternal(allocation,count);
}

extern "C" std::uint8_t* MEGUSTA_DTS_CDECL DTSLegacy_oldAlloc8_impl(
        std::uint8_t** allocation,int count) {
    return addressLegacyAlloc.bytes8.allocateExternal(allocation,count);
}

extern "C" std::uint32_t MEGUSTA_DTS_CDECL DTSLegacy_readAlloc32_append_impl(
        megusta::ResourceFile* file,std::uint32_t** allocation) {
    std::uint32_t value;
    file->read(&value,4,1);
    *addressLegacyAlloc.words32.allocateExternal(allocation,1)=value;
    return value;
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc32_store_impl(
        megusta::ResourceFile* file,std::uint32_t* value) {
    file->read(value,4,1);
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc_32_impl(
        megusta::ResourceFile* file,std::uint32_t** allocation,std::uint32_t count) {
    file->read(addressLegacyAlloc.words32.allocateExternal(allocation,static_cast<int>(count)),4,count);
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc_16_impl(
        megusta::ResourceFile* file,std::uint16_t** allocation,std::uint32_t count) {
    file->read(addressLegacyAlloc.words16.allocateExternal(allocation,static_cast<int>(count)),2,count);
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAlloc_8_impl(
        megusta::ResourceFile* file,std::uint8_t** allocation,std::uint32_t count) {
    file->read(addressLegacyAlloc.bytes8.allocateExternal(allocation,static_cast<int>(count)),1,count);
}

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_oldAllocGuard_impl(
        std::uint32_t** allocation32,std::uint16_t** allocation16,std::uint8_t** allocation8) {
    std::uint32_t* word=addressLegacyAlloc.words32.allocateExternal(allocation32,1);
    *word=addressLegacyAlloc.guard32;
    ++addressLegacyAlloc.guard32;
    std::uint16_t* shortWord=addressLegacyAlloc.words16.allocateExternal(allocation16,1);
    *shortWord=addressLegacyAlloc.guard16;
    ++addressLegacyAlloc.guard16;
    std::uint8_t* byte=addressLegacyAlloc.bytes8.allocateExternal(allocation8,1);
    *byte=addressLegacyAlloc.guard8;
    ++addressLegacyAlloc.guard8;
}
