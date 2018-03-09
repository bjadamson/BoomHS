#pragma once

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{
struct EngineState;
class LevelManager;
} // ns boomhs

namespace boomhs::ui_debug
{

void
draw(EngineState &, LevelManager &, window::SDLWindow &);

} // ns boomhs::ui_debug
