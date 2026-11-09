#include "mapped_resource_index.h"
#include "case_insensitive_path.h"
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cstdio>
#include <strings.h>
#endif
namespace megusta {
namespace {
const char* basename(const char* path) {
    const char* result = path;
    for (const char* p = path; *p; ++p)
        if (*p == '/' || *p == '\\') result = p + 1;
    return result;
}
}
// Original 0044d280: zero all nine state words.
MappedResourceIndex::MappedResourceIndex() noexcept
#if defined(_WIN32)
    : fileHandle_(nullptr), mappingHandle_(nullptr), mapping_(nullptr), entries_(nullptr),
#else
    : fileBytes_(), mapping_(nullptr), entries_(nullptr),
#endif
      entryCount_(0), hashSeed_(0), rotateLeft_(0), rotateRight_(0), modificationTime_(0) {}
// Original release alias 0044d2a0 and release 0044d3d0 share this behavior.
MappedResourceIndex::~MappedResourceIndex() { release(); }
void MappedResourceIndex::release() noexcept {
    std::free(entries_);
#if defined(_WIN32)
    if (mapping_) UnmapViewOfFile(mapping_);
    if (mappingHandle_) CloseHandle(mappingHandle_);
    if (fileHandle_) CloseHandle(fileHandle_);
    fileHandle_ = mappingHandle_ = nullptr;
#else
    fileBytes_.clear();
#endif
    mapping_ = nullptr; entries_ = nullptr; entryCount_ = 0;
    hashSeed_ = rotateLeft_ = rotateRight_ = 0; modificationTime_ = 0;
}
// Original 0044d2b0. Preserve its low-32-bit file sizing and header checks.
bool MappedResourceIndex::open(const char* path) {
    release();
    std::string resolvedPath;
    if (!engine_filesystem::resolvePathCaseInsensitive(path, resolvedPath)) return false;
#if defined(_WIN32)
    struct _stat status = {};
    _stat(resolvedPath.c_str(), &status);
#else
    struct stat status = {};
    stat(resolvedPath.c_str(), &status);
#endif
    modificationTime_ = static_cast<std::int32_t>(status.st_mtime);
#if defined(_WIN32)
    fileHandle_ = CreateFileA(resolvedPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle_ != INVALID_HANDLE_VALUE) {
        DWORD high;
        DWORD bytes = GetFileSize(fileHandle_, &high);
        if (bytes != 0) {
            mappingHandle_ = CreateFileMappingA(fileHandle_, nullptr, PAGE_READONLY, 0, bytes, nullptr);
            if (mappingHandle_ != INVALID_HANDLE_VALUE) {
                mapping_ = static_cast<const ResourceArchiveHeader*>(
                    MapViewOfFile(mappingHandle_, FILE_MAP_READ, 0, 0, 0));
                if (mapping_ && mapping_->magic == 0x0ba99a9e && mapping_->version == 1 &&
                    mapping_->entryCount > 0) {
                    hashSeed_ = mapping_->hashSeed;
                    rotateLeft_ = mapping_->rotateLeft;
                    rotateRight_ = 32 - rotateLeft_;
                    entryCount_ = mapping_->entryCount;
                    const std::uint32_t indexBytes = static_cast<std::uint32_t>(entryCount_) << 3;
                    entries_ = static_cast<ResourceHashEntry*>(std::malloc(indexBytes));
                    if (entries_) {
                        std::memcpy(entries_, mapping_ + 1, indexBytes);
                        return true;
                    }
                }
            }
        }
    }
#else
    FILE* file = std::fopen(resolvedPath.c_str(), "rb");
    if (file) {
        if (std::fseek(file, 0, SEEK_END) == 0) {
            const long length = std::ftell(file);
            if (length > 0 && std::fseek(file, 0, SEEK_SET) == 0) {
                fileBytes_.resize(static_cast<std::size_t>(length));
                if (std::fread(fileBytes_.data(), 1, fileBytes_.size(), file) == fileBytes_.size()) {
                    std::fclose(file);
                    file = nullptr;
                    mapping_ = reinterpret_cast<const ResourceArchiveHeader*>(fileBytes_.data());
                    if (mapping_->magic == 0x0ba99a9e && mapping_->version == 1 && mapping_->entryCount > 0) {
                        hashSeed_ = mapping_->hashSeed;
                        rotateLeft_ = mapping_->rotateLeft;
                        rotateRight_ = 32 - rotateLeft_;
                        entryCount_ = mapping_->entryCount;
                        const std::uint32_t indexBytes = static_cast<std::uint32_t>(entryCount_) << 3;
                        const std::size_t indexStart = sizeof(ResourceArchiveHeader);
                        if (indexStart + indexBytes <= fileBytes_.size()) {
                            entries_ = static_cast<ResourceHashEntry*>(std::malloc(indexBytes));
                            if (entries_) {
                                std::memcpy(entries_, fileBytes_.data() + indexStart, indexBytes);
                                return true;
                            }
                        }
                    }
                } else {
                    fileBytes_.clear();
                }
            }
        }
        if (file) std::fclose(file);
    }
#endif
    release();
    return false;
}
// Original 0044d420: the mask is deliberately 0x5f, not locale case folding.
bool MappedResourceIndex::hashBasename(const char* path, std::uint32_t& hash) const noexcept {
    if (!path) return false;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(basename(path));
    if (!*p) return false;
    std::uint32_t value = hashSeed_;
    while (*p) {
        value += *p++ & 0x5f;
        value = (value >> (rotateRight_ & 31)) | (value << (rotateLeft_ & 31));
    }
    hash = value;
    return true;
}
// Original 0044d4c0: unsigned hash comparisons and first matching midpoint.
int MappedResourceIndex::findHash(const char* path) const noexcept {
    if (!entryCount_) return -1;
    std::uint32_t hash;
    if (!hashBasename(path, hash)) return -1;
    int low = 0, high = entryCount_ - 1;
    while (low <= high) {
        int middle = (high + low) / 2;
        if (entries_[middle].hash < hash) low = middle + 1;
        else if (hash < entries_[middle].hash) high = middle - 1;
        else return middle;
    }
    return -1;
}
// Original 0044d540: an existence-only query deliberately skips name validation.
bool MappedResourceIndex::findResource(const char* path, const void** data, std::uint32_t* size) const {
    int index = findHash(path);
    if (index < 0) return false;
    if (data || size) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(mapping_);
        const auto* record = reinterpret_cast<const ResourceRecordHeader*>(bytes + entries_[index].recordOffset);
#if defined(_WIN32)
        if (_strnicmp(basename(path), record->basename, 15)) return false;
#else
        if (strncasecmp(basename(path), record->basename, 15)) return false;
#endif
        if (size) *size = record->byteCount;
        if (data) *data = record + 1;
    }
    return true;
}
}
