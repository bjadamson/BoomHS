#pragma once
#include <extlibs/sdl.hpp>

namespace window
{
class FrameTime;
} // ns window

namespace boomhs
{
struct EngineState;
struct GameState;
} // ns boomhs

namespace boomhs::main_menu
{

void
draw(EngineState&);

bool
process_event(GameState&, SDL_Event &, window::FrameTime const&);

} // ns boomhs::main_menu
