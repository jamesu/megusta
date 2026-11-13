// SPDX-License-Identifier: GPL-2.0

#pragma once
namespace megusta { class ResourceFile; namespace dts {
// The owner must unbind before destroying the service.
void bindIflResourceService(ResourceFile* (*open)(const char*,const char*),int (*close)(ResourceFile*));
} }
