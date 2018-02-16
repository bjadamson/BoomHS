#include <boomhs/zone.hpp>
#include <boomhs/state.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// ZoneManager
ZoneManager::ZoneManager(ZoneStates &&zs)
  : zstates_(MOVE(zs))
{
}

ZoneState const&
ZoneManager::active() const
{
  return zstates_[active_];
}

ZoneState&
ZoneManager::active()
{
  return zstates_[active_];
}

void
ZoneManager::make_zone_active(int const zone_number, TiledataState &tds)
{
  active_ = zone_number;
  tds.recompute = true;
}

int
ZoneManager::num_zones() const
{
  return zstates_.size();
}

int
ZoneManager::active_zone() const
{
  return active_;
}

} // ns boomhs
