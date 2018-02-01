#pragma once
#include <boomhs/state.hpp>

namespace boomhs
{

class ZoneManager
{
  ZoneStates &zstates_;
public:
  explicit ZoneManager(ZoneStates &zs)
    : zstates_(zs)
  {
  }

  auto const&
  active() const
  {
    auto const& data = zstates_.data();
    auto const active = zstates_.active();
    return data[active];
  }

  auto&
  active()
  {
    auto &data = zstates_.data();
    auto const active = zstates_.active();
    return data[active];
  }

  void
  make_zone_active(int const zone_number, GameState &state)
  {
    zstates_.set_active(zone_number);

    auto &tm_state = state.engine_state.tilemap_state;
    tm_state.recompute = true;
  }

  auto
  num_zones() const
  {
    return zstates_.size();
  }

  int
  active_zone() const
  {
    return zstates_.active();
  }
};

} // ns boomhs
