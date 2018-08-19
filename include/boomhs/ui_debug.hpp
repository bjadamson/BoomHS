#pragma once
#include <boomhs/clock.hpp>

namespace opengl
{
struct DrawState;
} // namespace opengl

namespace window
{
class SDLWindow;
} // namespace window

namespace boomhs
{
class Camera;
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
     Camera&, opengl::DrawState&, FrameTime const&);

} // namespace boomhs::ui_debug
