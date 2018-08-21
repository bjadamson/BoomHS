#pragma once
#include <boomhs/clock.hpp>

namespace boomhs
{
class Camera;
struct EngineState;
class LevelManager;
class WaterAudioSystem;
} // namespace boomhs

namespace opengl
{
class SkyboxRenderer;
} // namespace opengl

namespace boomhs::ui_debug
{

void
draw(EngineState&, LevelManager&, opengl::SkyboxRenderer&, WaterAudioSystem&, Camera&,
     FrameTime const&);

} // namespace boomhs::ui_debug
