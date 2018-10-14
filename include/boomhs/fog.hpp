#pragma once
#include <boomhs/color.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{

struct Fog
{
  float density;
  float gradient;

  Color color;

  Fog() = default;
  MOVE_CONSTRUCTIBLE_ONLY(Fog);
};

} // namespace boomhs
