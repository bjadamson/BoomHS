#pragma once
#include <extlibs/sdl.hpp>

struct ImVec2;

namespace boomhs
{
class  Camera;
struct EngineState;
class  FrameTime;
struct GameState;
class  PlayerBehavior;
class  SDLEventProcessArgs;
struct ZoneState;
class  WaterAudioSystem;

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
process_event(SDLEventProcessArgs &&);

} // namespace boomhs::main_menu
