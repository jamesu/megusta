#include "gl_token_table.h"
#include <GL/gl.h>
namespace megusta {
Win32GLTokenTable::Win32GLTokenTable() {
    for (auto& value:values) value=unmapped;
#include "gl_token_values.inc"
    values[0xa8]=0x1f03u; // GL_EXTENSIONS, also available to the browser GL shim.
}
}
