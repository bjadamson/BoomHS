#pragma once
#include <common/type_macros.hpp>
#include <boomhs/colors.hpp>

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
