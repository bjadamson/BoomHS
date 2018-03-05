#include <boomhs/nearby_targets.hpp>

namespace boomhs
{

void
NearbyTargets::add_target(EntityID const t)
{
  targets_.emplace_back(t);
}

void
NearbyTargets::clear()
{
  targets_.clear();
}

void
NearbyTargets::cycle_forward()
{
  offset_ += 1;
  if (offset_ >= targets_.size()) {
    offset_ = 0;
  }
}

void
NearbyTargets::cycle_backward()
{
  if (offset_ == 0) {
    offset_ = targets_.size();
  }
  offset_ -= 1;
}

bool
NearbyTargets::empty() const
{
  return targets_.empty();
}

EntityID
NearbyTargets::closest() const
{
  return targets_[offset_];
}

} // ns boomhs