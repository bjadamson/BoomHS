#pragma once
#include <boomhs/types.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct skybox
{
  Transform &transform;
  MOVE_CONSTRUCTIBLE_ONLY(skybox);
};

} // ns boomhs
