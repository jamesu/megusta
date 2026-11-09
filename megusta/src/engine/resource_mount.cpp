#include "resource_mount.h"
#include "case_insensitive_path.h"
#include "string_services.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include "../game/config_literals.h"
namespace megusta {
namespace {
// 0052ce70: shared transient path buffer, overwritten on each directory match.
char translatedPath[256];
int queryNativeStat(const char* path, ResourceStatus& result) {
    std::string resolved;
    if (!engine_filesystem::resolvePathCaseInsensitive(path, resolved)) return -1;
#if defined(_WIN32)
    struct _stat native;
    if (_stat(resolved.c_str(), &native) != 0) return -1;
#else
    struct stat native;
    if (stat(resolved.c_str(), &native) != 0) return -1;
#endif
    result.byteCount = static_cast<std::int32_t>(native.st_size);
    result.modificationTime = static_cast<std::int32_t>(native.st_mtime);
    result.kindFlags = S_ISDIR(native.st_mode) ? 1 : S_ISREG(native.st_mode) ? 2 : 0;
    return 0;
}
}
// 0043af50: classify the physical source before publishing an entry.
bool ResourceMountTable::add(const char* virtualPrefix, const char* physicalPath) {
    if (entryCount_ >= 20) return false;
    ResourceStatus status;
    if (queryNativeStat(physicalPath, status) != 0) return false;
    std::unique_ptr<MappedResourceIndex> archive;
    if (!(status.kindFlags & 1)) {
        archive.reset(new MappedResourceIndex);
        if (!archive->open(physicalPath)) return false;
    }
    ResourceMountEntry& entry = entries_[entryCount_++];
    entry.virtualPrefix = virtualPrefix;
    entry.physicalPath = physicalPath;
    entry.archive = std::move(archive);
    return true;
}
// 0043b080: prefix matching is insensitive and does not require a separator.
const char* ResourceMountEntry::translatePrefix(const char* path) const {
    const std::size_t length = virtualPrefix.size();
    if (strings::compareInsensitiveBounded(path, virtualPrefix.c_str(), length)) return nullptr;
    if (archive) return path + length;
    // Original uses an unbounded formatter into this same fixed-size buffer.
    std::sprintf(translatedPath, config_literals::mountFormat, physicalPath.c_str(), path + length);
    return translatedPath;
}
}
namespace megusta {
// 0043a480: search only registered mounts, preserving their insertion order.
std::unique_ptr<ResourceFile> ResourceMountTable::open(const char* path, const char* mode) const {
    for (int i = 0; i < entryCount_; ++i) {
        const ResourceMountEntry& mount = entries_[i];
        const char* translated = mount.translatePrefix(path);
        if (!translated) continue;
        if (!mount.archive) {
            if (auto* file = engine_filesystem::openFileCaseInsensitive(translated, mode))
                return std::unique_ptr<ResourceFile>(new ResourceFile(file));
        } else if (std::strchr(mode, 'r')) {
            const void* data;
            std::uint32_t size;
            if (!mount.archive->findResource(translated, &data, &size)) continue;
            void* owned = nullptr;
            if (!std::strchr(mode, 'b')) {
                const auto* start = static_cast<const unsigned char*>(data);
                const auto* endMarker = static_cast<const unsigned char*>(std::memchr(start, 0x1a, size));
                if (endMarker) size = static_cast<std::uint32_t>(endMarker - start);
                owned = std::malloc(size);
                auto* output = static_cast<unsigned char*>(owned);
                const auto* end = start + size;
                while (start != end) {
                    if (*start == '\r' && start + 1 != end && start[1] == '\n') {
                        *output++ = '\n'; start += 2;
                    } else *output++ = *start++;
                }
                data = owned;
                size = static_cast<std::uint32_t>(output - static_cast<unsigned char*>(owned));
            }
            return std::unique_ptr<ResourceFile>(new ResourceFile(data, static_cast<std::int32_t>(size), owned));
        }
    }
    return nullptr;
}
}

namespace megusta {
// 0043a3f0: mounted archive records report regular-file kind and archive timestamp.
int ResourceMountTable::queryStat(const char* path, ResourceStatus& status) const {
    for (int i = 0; i < entryCount_; ++i) {
        const auto& mount = entries_[i];
        const char* translated = mount.translatePrefix(path);
        if (!translated) continue;
        if (!mount.archive) {
            if (queryNativeStat(translated, status) == 0) return 0;
        } else {
            std::uint32_t size;
            if (mount.archive->findResource(translated, nullptr, &size)) {
                status.kindFlags = 2;
                status.byteCount = static_cast<std::int32_t>(size);
                status.modificationTime = mount.archive->modificationTime();
                return 0;
            }
        }
    }
    return -1;
}
}
