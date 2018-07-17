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
class SkyboxRenderer;
} // namespace boomhs

namespace boomhs::ui_debug
{

void
draw(EngineState&, LevelManager&, SkyboxRenderer&, window::SDLWindow&, Camera&, DrawState&,
     window::FrameTime const&);

} // namespace boomhs::ui_debug
