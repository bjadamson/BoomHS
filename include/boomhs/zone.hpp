#pragma once
#include <boomhs/zone_state.hpp>
#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{
struct TiledataState;

class ZoneManager
{
  ZoneStates zstates_;
  int active_ = 0;
public:
  MOVE_CONSTRUCTIBLE_ONLY(ZoneManager);
  explicit ZoneManager(ZoneStates &&);

  ZoneState const& active() const;
  ZoneState& active();

  void make_zone_active(int const zone_number, TiledataState &tds);
  int num_zones() const;
  int active_zone() const;

  /*
  void
  add_zone(ZoneState &&zone)
  {
    zstates_.add_zone(MOVE(zone));
  }
  */
};

} // ns boomhs
