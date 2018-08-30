#pragma once

namespace boomhs
{
class Camera;
struct EngineState;
class LevelManager;
} // namespace boomhs

namespace boomhs::ui_debug
{

void
draw(char const*, int, EngineState&, LevelManager&, Camera&, FrameTime const&);

} // namespace boomhs::ui_debug
