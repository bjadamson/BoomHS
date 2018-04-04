#pragma once
#include <boomhs/entity.hpp>
#include <vector>

namespace boomhs
{

class NearbyTargets
{
  size_t                offset_  = 0;
  std::vector<EntityID> targets_ = {};

public:
  NearbyTargets() = default;

  void add_target(EntityID);
  void clear();
  void cycle_forward();
  void cycle_backward();

  bool     empty() const;
  EntityID closest() const;
};

} // namespace boomhs
