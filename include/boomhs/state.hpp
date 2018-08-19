#pragma once
#include <boomhs/level_manager.hpp>

namespace boomhs
{
struct EngineState;

struct GameState
{
  EngineState& engine_state;
  LevelManager level_manager;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);

  explicit GameState(EngineState&, LevelManager&&);
};

} // namespace boomhs
