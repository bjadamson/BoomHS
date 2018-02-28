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
draw_ui(EngineState &, LevelManager &, window::SDLWindow &, EntityRegistry &);

} // ns boomhs
