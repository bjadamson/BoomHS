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
};

} // ns boomhs
