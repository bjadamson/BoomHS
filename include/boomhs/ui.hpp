#pragma once
#include <entt/entt.hpp>

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{

struct GameState;
void
draw_ui(GameState &, window::SDLWindow &, entt::DefaultRegistry &);

} // ns boomhs
