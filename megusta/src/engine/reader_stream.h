// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <GL/megusta_reader.h>
#include "resource_file.h"
namespace megusta {
inline MegustaReader resourceReader(ResourceFile& file) {
    return {&file,
        [](void* context,void* output,std::size_t size,std::size_t count)->std::size_t {
            return static_cast<ResourceFile*>(context)->read(output,static_cast<std::uint32_t>(size),static_cast<std::uint32_t>(count));
        },
        [](void* context,long offset,int origin) {
            return static_cast<ResourceFile*>(context)->seek(static_cast<std::int32_t>(offset),origin);
        }};
}
}
