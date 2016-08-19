// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef SPDLOG_INCLUDE_ONCE_ONLY
#define SPDLOG_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/spdlog.hpp should only be included once per-project.");
#endif // SPDLOG_INCLUDE_ONCE_ONLY
#define SPDLOG_TRACE_ON
#include <spdlog/spdlog.h>
