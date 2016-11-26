// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef COMPACT_OPTIONAL_INCLUDE_ONCE_ONLY
#define COMPACT_OPTIONAL_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/compact_optional.hpp should only be included once per-project.");
#endif // COMPACT_OPTIONAL_INCLUDE_ONCE_ONLY
#include <compact_optional.hpp>
