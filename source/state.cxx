#include <boomhs/state.hpp>
#include <boomhs/zone.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// ZoneStates
ZoneState&
ZoneStates::active()
{
  return zstates_[active_zone()];
}

ZoneState const&
ZoneStates::active() const
{
  return zstates_[active_zone()];
}

int
ZoneStates::active_zone() const
{
  assert(active_ < size());
  return active_;
}

int
ZoneStates::size() const
{
  return zstates_.size();
}

/*
void
ZoneStates::add_zone(ZoneState &&zs)
{
  zstates_.emplace_back(MOVE(zs));
}
*/

void
ZoneStates::set_active(int const zone_number)
{
  assert(zone_number < size());
  active_ = zone_number;
}


ZoneState&
ZoneStates::operator[](size_t const i)
{
  assert(i < zstates_.size());
  return zstates_[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GameState
GameState::GameState(EngineState &&es, ZoneStates &&zs)
  : engine_state(MOVE(es))
  , zone_states(MOVE(zs))
{
}

RenderArgs
GameState::render_args()
{
  auto &logger = engine_state.logger;

  ZoneManager zm{zone_states};
  auto &active = zm.active();

  auto const& camera = active.camera;
  auto const& player = active.player;
  auto const& global_light = active.global_light;

  bool const draw_normals = engine_state.draw_normals;
  return RenderArgs{camera, player, logger, global_light, draw_normals};
}

} // ns boomhs
