#pragma once
#include <boomhs/state.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

class ZoneManager
{
  ZoneStates &zstates_;
public:
  NO_COPY_AND_NO_MOVE(ZoneManager);
  explicit ZoneManager(ZoneStates &zs)
    : zstates_(zs)
  {
  }

  ZoneState const&
  active() const
  {
    return zstates_.active();
  }

  ZoneState&
  active()
  {
    return zstates_.active();
  }

  void
  make_zone_active(int const zone_number, GameState &state)
  {
    zstates_.set_active(zone_number);

    auto &tm_state = state.engine_state.tilemap_state;
    tm_state.recompute = true;
  }

  int
  num_zones() const
  {
    return zstates_.size();
  }

  int
  active_zone() const
  {
    return zstates_.active_zone();
  }
};

} // ns boomhs
