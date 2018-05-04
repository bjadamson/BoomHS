#pragma once
#include <extlibs/sdl.hpp>

struct ImVec2;

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class Camera;
struct EngineState;
struct GameState;
} // namespace boomhs

namespace boomhs::main_menu
{

void
draw(EngineState&, ImVec2 const&);

void
process_event(GameState&, SDL_Event&, Camera&, window::FrameTime const&);

} // namespace boomhs::main_menu
