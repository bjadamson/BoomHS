#pragma once
#include <extlibs/sdl.hpp>

namespace opengl
{
struct DrawState;
class SkyboxRenderer;
} // namespace opengl

namespace gl_sdl
{
class SDLWindow;
} // namespace gl_sdl

namespace boomhs
{
class  Camera;
class  Viewport;
struct EngineState;
class  FrameTime;
class  GameState;
class  LevelManager;
struct PlayerBehavior;
struct SDLEventProcessArgs;
struct WaterAudioSystem;

struct MainMenuState
{
  bool show         = true;
  bool show_options = false;
};

} // namespace boomhs

namespace boomhs::main_menu
{

void
draw(EngineState&, gl_sdl::SDLWindow&, Camera&, opengl::SkyboxRenderer&, opengl::DrawState&,
     LevelManager&, Viewport const&, WaterAudioSystem&);

void
process_event(SDLEventProcessArgs&&);

} // namespace boomhs::main_menu
