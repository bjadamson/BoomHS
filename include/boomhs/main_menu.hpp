#pragma once
#include <extlibs/sdl.hpp>

struct ImVec2;

namespace boomhs
{
class Camera;
class FrameTime;
class WaterAudioSystem;
struct EngineState;
struct GameState;
struct ZoneState;

struct MainMenuState
{
  bool show         = true;
  bool show_options = false;
};

} // namespace boomhs

namespace boomhs::main_menu
{

void
draw(EngineState&, ZoneState&, ImVec2 const&, WaterAudioSystem&);

void
process_event(GameState&, SDL_Event&, Camera&, FrameTime const&);

} // namespace boomhs::main_menu
