#pragma once
#include <boomhs/state.hpp>

#include <common/result.hpp>
#include <string>

namespace opengl
{
struct DrawState;
struct RenderState;
} // namespace opengl

namespace boomhs
{
class FrameTime;
class Camera;
class Engine;
class RNG;

Result<GameState, std::string>
init(Engine&, EngineState&, Camera&, RNG&);

void
game_loop(Engine&, GameState&, RNG&, Camera&, FrameTime const&);

} // namespace boomhs
