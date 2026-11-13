// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "mapped_resource_index.h"
#include "resource_file.h"
#include "file_status.h"
#include <array>
#include <memory>
#include <string>
namespace megusta {
using ResourceStatus=EngineFileStatus;
struct ResourceMountEntry {
    std::string virtualPrefix;
    std::string physicalPath;
    std::unique_ptr<MappedResourceIndex> archive;
    const char* translatePrefix(const char* path) const;
};
class ResourceMountTable {
public:
    void resetMountCount() noexcept { entryCount_ = 0; }
    bool add(const char* virtualPrefix, const char* physicalPath);
    bool prepend(const char* virtualPrefix, const char* physicalPath);
    std::unique_ptr<ResourceFile> open(const char* path, const char* mode) const;
    // Try suffix alternatives within each mount before falling back to a lower
    // priority mount (e.g. a mod's bytecode must override base Python source).
    std::unique_ptr<ResourceFile> openFirst(const char* const* paths, std::size_t count,
        const char* mode, std::size_t* selected = nullptr) const;
    int queryStat(const char* path, ResourceStatus& status) const;
private:
    std::array<ResourceMountEntry, 20> entries_;
    int entryCount_ = 0;
};
}
