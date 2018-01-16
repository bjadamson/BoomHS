// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef ENTT_INCLUDE_ONCE_ONLY
#define ENTT_INCLUDE_ONCE_ONLY
#define ENTT_HEADER_ONLY
#include <entt/entt.hpp>
#else
static_assert(false, "extlibs/entt.hpp should only be included once per-project.");
#endif // ENTT_INCLUDE_ONCE_ONLY
