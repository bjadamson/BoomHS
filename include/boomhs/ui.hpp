#pragma once

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{
class EntityRegistry;

struct EngineState;
class LevelManager;

void
draw_ingame_ui(EngineState &, LevelManager &, EntityRegistry &);

void
draw_debug_ui(EngineState &, LevelManager &, window::SDLWindow &, EntityRegistry &);

} // ns boomhs
