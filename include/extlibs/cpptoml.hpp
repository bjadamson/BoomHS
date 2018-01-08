// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef CPPTOML_INCLUDE_ONCE_ONLY
#define CPPTOML_INCLUDE_ONCE_ONLY
#define CPPTOML_HEADER_ONLY
#include <cpptoml/cpptoml.h>
#else
static_assert(false, "extlibs/cpptoml.hpp should only be included once per-project.");
#endif // FMT_INCLUDE_ONCE_ONLY
