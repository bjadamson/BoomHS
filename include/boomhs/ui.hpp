#pragma once

namespace window
{
class SDLWindow;
} // ns window

namespace boomhs
{

struct GameState;
void
draw_ui(GameState &state, window::SDLWindow &window);

} // ns boomhs
