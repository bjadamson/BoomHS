#include <boomhs/state.hpp>
#include <boomhs/zone.hpp>

namespace boomhs
{

GameState::GameState(EngineState &&es, ZoneManager &&zm)
  : engine_state(MOVE(es))
  , zone_manager(MOVE(zm))
{
}

} // ns boomhs
