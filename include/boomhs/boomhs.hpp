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
class  FrameTime;
class  Camera;
struct Engine;
class  RNG;
struct WorldOrientation;

Result<GameState, std::string>
create_gamestate(Engine&, EngineState&, WorldOrientation const&, Camera&, RNG&);

void
init_gamestate_inplace(GameState&, Camera&);

void
game_loop(Engine&, GameState&, RNG&, Camera&, FrameTime const&);

} // namespace boomhs
