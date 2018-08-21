#pragma once
#include <extlibs/sdl.hpp>

struct ImVec2;

namespace opengl
{
struct DrawState;
class SkyboxRenderer;
} // namespace opengl

namespace window
{
class SDLWindow;
} // namespace window

namespace boomhs
{
class  Camera;
struct EngineState;
class  FrameTime;
struct GameState;
struct LevelManager;
class  PlayerBehavior;
class  SDLEventProcessArgs;
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
draw(EngineState&, window::SDLWindow&, Camera&, opengl::SkyboxRenderer&, opengl::DrawState&,
     LevelManager&, ImVec2 const&, WaterAudioSystem&);

void
process_event(SDLEventProcessArgs &&);

} // namespace boomhs::main_menu
