#pragma once
#include <boomhs/types.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct Skybox
{
  Transform &transform;
  MOVE_CONSTRUCTIBLE_ONLY(Skybox);
};

} // ns boomhs
