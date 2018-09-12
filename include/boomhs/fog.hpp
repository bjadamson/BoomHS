#pragma once
#include <boomhs/colors.hpp>
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
