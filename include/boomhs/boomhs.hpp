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
} // ns stlw

namespace boomhs
{

Result<GameState, std::string>
init(Engine&, EngineState&);

void
game_loop(Engine&, GameState&, stlw::float_generator&, window::FrameTime const&);

} // ns boomhs
