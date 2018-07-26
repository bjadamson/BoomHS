#pragma once
#include <boomhs/zone_state.hpp>
#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{
class LevelManager
{
  ZoneStates zstates_;
  int        active_ = 0;

public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelManager);
  explicit LevelManager(ZoneStates&&);

  ZoneState const& active() const;
  ZoneState&       active();

  void make_active(int);
  int  num_levels() const;
  int  active_zone() const;

  BEGIN_END_FORWARD_FNS(zstates_);

  /*
  void
  add_zone(ZoneState &&zone)
  {
    zstates_.add_zone(MOVE(zone));
  }
  */
};

} // namespace boomhs
