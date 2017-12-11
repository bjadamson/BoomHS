// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef EXPECTED_INCLUDE_ONCE_ONLY
#define EXPECTED_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/expected.hpp should only be included once per-project.");
#endif // EXPECTED_INCLUDE_ONCE_ONLY
#include <expected/expected.hpp>
