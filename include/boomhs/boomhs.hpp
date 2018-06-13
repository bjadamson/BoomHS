#pragma once
#include <boomhs/state.hpp>

#include <stlw/result.hpp>
#include <string>

namespace window
{
class FrameTime;
}

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{
class Camera;

Result<GameState, std::string>
create_gamestate(Engine&, EngineState&, Camera&);

void
init(Engine&, GameState&);

void
game_loop(Engine&, GameState&, stlw::float_generator&, Camera&, window::FrameTime const&);

} // namespace boomhs
