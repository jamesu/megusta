#include "scene_record_accessors.h"

namespace megusta {

void* SceneRecord_getPointAt10_impl(void* record) {
    return static_cast<unsigned char*>(record) + 0x10;
}

} // namespace megusta
