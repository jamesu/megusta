#pragma once
#include <cstdint>
#include <windows.h>
namespace megusta {
// Serialized RBX records. These are file contracts, not C++ object ABI views.
struct ResourceArchiveHeader {
    std::uint32_t magic, version;
    std::int32_t entryCount;
    std::uint32_t hashSeed, rotateLeft;
};
struct ResourceHashEntry { std::uint32_t hash, recordOffset; };
struct ResourceRecordHeader {
    std::uint32_t unknown;
    std::uint32_t byteCount;
    char basename[16];
};
static_assert(sizeof(ResourceArchiveHeader) == 20, "RBX header");
static_assert(sizeof(ResourceHashEntry) == 8, "RBX index entry");
static_assert(sizeof(ResourceRecordHeader) == 24, "RBX resource header");
class MappedResourceIndex {
public:
    MappedResourceIndex() noexcept;
    ~MappedResourceIndex();
    MappedResourceIndex(const MappedResourceIndex&) = delete;
    MappedResourceIndex& operator=(const MappedResourceIndex&) = delete;
    bool open(const char* path);
    void release() noexcept;
    bool hashBasename(const char* path, std::uint32_t& hash) const noexcept;
    int findHash(const char* path) const noexcept;
    std::int32_t modificationTime() const noexcept { return modificationTime_; }
    bool findResource(const char* path, const void** data, std::uint32_t* size) const;
private:
    HANDLE fileHandle_;
    HANDLE mappingHandle_;
    const ResourceArchiveHeader* mapping_;
    ResourceHashEntry* entries_;
    std::int32_t entryCount_;
    std::uint32_t hashSeed_, rotateLeft_, rotateRight_;
    std::int32_t modificationTime_;
};
}
