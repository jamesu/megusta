#include "opengl_adapter.h"
#include "gl_token_table.h"
#include "platform_gl.h"
namespace megusta {
bool OpenGLAdapter::isEnabled(int n) { return glIsEnabled(tokens[n])!=0; }
}
