// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef IMGUI_INCLUDE_ONCE_ONLY
#define IMGUI_INCLUDE_ONCE_ONLY
#define IMGUI_HEADER_ONLY
#include <imgui/imgui.h>
#else
static_assert(false, "extlibs/imgui.hpp should only be included once per-project.");
#endif // IMGUI_INCLUDE_ONCE_ONLY
