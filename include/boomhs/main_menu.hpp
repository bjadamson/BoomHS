#pragma once
#include <extlibs/sdl.hpp>

struct ImVec2;

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
draw(EngineState&, ImVec2 const&);

void
process_event(GameState&, SDL_Event &, window::FrameTime const&);

} // ns boomhs::main_menu
