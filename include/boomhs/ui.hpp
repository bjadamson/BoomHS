#pragma once
#include <entt/entt.hpp>

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{

struct EngineState;
class ZoneManager;
void
draw_ui(EngineState &, ZoneManager &, window::SDLWindow &, entt::DefaultRegistry &);

} // ns boomhs
