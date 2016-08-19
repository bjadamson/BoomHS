// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef FMT_INCLUDE_ONCE_ONLY
#define FMT_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/fmt.hpp should only be included once per-project.");
#endif // FMT_INCLUDE_ONCE_ONLY
#include <fmt/format.h>
