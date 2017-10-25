// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#pragma once
#ifndef TINYOBJ_INCLUDE_ONCE_ONLY
#define TINYOBJ_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/tinyobj.hpp should only be included once per-project.");
#endif // TINYOBJ_INCLUDE_ONCE_ONLY
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tinyobj.hpp>
