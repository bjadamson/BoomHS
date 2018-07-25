#pragma once
#include <window/timer.hpp>

namespace window
{
class SDLWindow;
} // namespace window

namespace boomhs
{
class Camera;
struct DrawState;
struct EngineState;
class LevelManager;
class WaterAudioSystem;
} // namespace boomhs

namespace opengl
{
class SkyboxRenderer;
} // namespace opengl

namespace boomhs::ui_debug
{

void
draw(EngineState&, LevelManager&, opengl::SkyboxRenderer&, WaterAudioSystem&, window::SDLWindow&,
     Camera&, DrawState&, window::FrameTime const&);

} // namespace boomhs::ui_debug
