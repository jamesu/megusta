#include "gl_token_table.h"
#include <windows.h>
#include <GL/gl.h>
namespace megusta {
Win32GLTokenTable::Win32GLTokenTable() {
    for (auto& value:values) value=unmapped;
#include "gl_token_values.inc"
    values[0xa8]=GL_EXTENSIONS;
}
}
