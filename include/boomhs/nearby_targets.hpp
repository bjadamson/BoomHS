#pragma once
#include <vector>

namespace boomhs
{

class NearbyTargets
{
  size_t offset_ = 0;
  std::vector<uint32_t> targets_ = {};

public:
  NearbyTargets() = default;

  void add_target(uint32_t);
  void clear();
  void cycle_forward();
  void cycle_backward();

  bool empty() const;
  uint32_t closest() const;
};

} // ns boomhs
