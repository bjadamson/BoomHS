#pragma once

namespace window
{
class SDLWindow;
} // namespace window

namespace boomhs
{
struct EngineState;
class LevelManager;
} // namespace boomhs

namespace boomhs::ui_debug
{

void
draw(EngineState&, LevelManager&, window::SDLWindow&);

} // namespace boomhs::ui_debug
