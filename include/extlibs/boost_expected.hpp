// We can't put this block of code into a macro, because the spec doesn't allow us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this directory.
#ifndef BOOST_EXPECTED_INCLUDE_ONCE_ONLY
#define BOOST_EXPECTED_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/boost_expected.hpp should only be included once per-project.");
#endif // BOOST_EXPECTED_INCLUDE_ONCE_ONLY
#include <boost/expected/expected.hpp>
