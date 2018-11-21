#include <boomhs/engine.hpp>
#include <boomhs/state.hpp>

namespace boomhs
{

GameState::GameState(EngineState& es, LevelManager&& lm, WaterAudioSystem&& was)
    : es_(es)
    , lm_(MOVE(lm))
    , was_(MOVE(was))
{
}

} // namespace boomhs
