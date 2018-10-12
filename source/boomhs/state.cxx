#include <boomhs/state.hpp>
#include <boomhs/engine.hpp>

namespace boomhs
{

GameState::GameState(EngineState& es, LevelManager&& lm)
    : es_(es)
    , lm_(MOVE(lm))
{
}

} // namespace boomhs
