#include <boomhs/level_manager.hpp>
#include <boomhs/state.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// LevelManager
LevelManager::LevelManager(ZoneStates&& zs)
    : zstates_(MOVE(zs))
{
}

ZoneState const&
LevelManager::active() const
{
  return zstates_[active_];
}

ZoneState&
LevelManager::active()
{
  return zstates_[active_];
}

void
LevelManager::make_active(int const level_number, TiledataState& tds)
{
  active_ = level_number;
  tds.recompute = true;
}

int
LevelManager::num_levels() const
{
  return zstates_.size();
}

int
LevelManager::active_zone() const
{
  return active_;
}

} // namespace boomhs
