#include <boomhs/state.hpp>
#include <boomhs/engine.hpp>

namespace boomhs
{

GameState::GameState(EngineState& es, LevelManager&& lm)
    : engine_state(es)
    , level_manager(MOVE(lm))
{
}

} // namespace boomhs
