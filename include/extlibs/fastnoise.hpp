// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef FASTNOISE_INCLUDE_ONCE_ONLY
#define FASTNOISE_INCLUDE_ONCE_ONLY
#define FASTNOISE_HEADER_ONLY
#include <fastnoise/fastnoise.hpp>
#else
static_assert(false, "extlibs/fastnoise.hpp should only be included once per-project.");
#endif // FASTNOISE_INCLUDE_ONCE_ONLY
