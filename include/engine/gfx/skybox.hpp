#pragma once
#include <game/data_types.hpp>
#include <stlw/type_macros.hpp>

namespace engine::gfx
{

struct skybox
{
  model &model;

  MOVE_CONSTRUCTIBLE_ONLY(skybox);
};

} // ns engine::gfx
