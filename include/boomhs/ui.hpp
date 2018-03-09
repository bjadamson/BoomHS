#pragma once

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{

struct EngineState;
class LevelManager;

void
draw_ingame_ui(EngineState &, LevelManager &);

void
draw_debug_ui(EngineState &, LevelManager &, window::SDLWindow &);

} // ns boomhs
