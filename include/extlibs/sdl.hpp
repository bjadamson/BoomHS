// We can't put this block of code into a macro, because the spec doesn't allow
// us to generate
// macros from macros. Therefore, we copy/paste it to each header file in this
// directory.
#ifndef SDL_INCLUDE_ONCE_ONLY
#define SDL_INCLUDE_ONCE_ONLY
#else
static_assert(false, "extlibs/sdl.hpp should only be included once per-project.");
#endif // SDL_INCLUDE_ONCE_ONLY

// SDL headers
#include <SDL.h>
#include <SDL_main.h>
