#pragma once
#include <window/timer.hpp>

namespace window
{
class SDLWindow;
} // namespace window

namespace boomhs
{
class Camera;
struct EngineState;
class LevelManager;
} // namespace boomhs

namespace boomhs::ui_debug
{

void
draw(EngineState&, LevelManager&, window::SDLWindow&, Camera&, window::FrameTime const&);

} // namespace boomhs::ui_debug
