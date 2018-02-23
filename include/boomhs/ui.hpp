#pragma once

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{
class EntityRegistry;

struct EngineState;
class ZoneManager;
void
draw_ui(EngineState &, ZoneManager &, window::SDLWindow &, EntityRegistry &);

} // ns boomhs
