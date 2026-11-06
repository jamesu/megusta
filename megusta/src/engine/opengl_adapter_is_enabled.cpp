#include "opengl_adapter.h"
#include "gl_token_table.h"
#include <windows.h>
#include <GL/gl.h>
namespace megusta {
bool OpenGLAdapter::isEnabled(int n) { return glIsEnabled(tokens[n])!=0; }
}
