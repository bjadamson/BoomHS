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
class Camera;
struct ScreenViewport;
struct EngineState;
class FrameTime;
struct GameState;
struct LevelManager;
class PlayerBehavior;
class SDLEventProcessArgs;
class WaterAudioSystem;

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
     LevelManager&, ScreenViewport const&, WaterAudioSystem&);

void
process_event(SDLEventProcessArgs&&);

} // namespace boomhs::main_menu
