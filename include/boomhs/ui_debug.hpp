#pragma once
#include <boomhs/clock.hpp>

namespace boomhs
{
class Camera;
struct EngineState;
class LevelManager;
} // namespace boomhs

namespace boomhs::ui_debug
{

void
draw(EngineState&, LevelManager&, Camera&, FrameTime const&);

} // namespace boomhs::ui_debug
