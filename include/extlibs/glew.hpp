// We can't put this block of code into a macro, because the spec doesn't allow us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this directory.
#ifndef GLEW_INCLUDE_ONCE_ONLY
#define GLEW_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/glew.hpp should only be included once per-project.");
#endif // GLEW_INCLUDE_ONCE_ONLY

#define GLEW_STATIC
#include <GL/glew.h>

// OpenGL headers
#include <GL/glu.h>
#include <GL/gl.h>
