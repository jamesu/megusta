// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "putt_map.h"
namespace megusta {
class ResourceMountTable;
// The mount table and this adapter must outlive any load using services().
class PuttMapResourceIO {
public:
    explicit PuttMapResourceIO(ResourceMountTable& mounts);
    PuttMapResourceIO(const PuttMapResourceIO&)=delete;
    PuttMapResourceIO& operator=(const PuttMapResourceIO&)=delete;
    PuttMapIO& services() { return services_; }
    ResourceMountTable& mounts() { return mounts_; }
private:
    ResourceMountTable& mounts_;
    PuttMapIO services_;
};
}
