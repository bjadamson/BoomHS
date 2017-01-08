#pragma once
#include <opengl/types.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

struct skybox
{
  model &model;
  MOVE_CONSTRUCTIBLE_ONLY(skybox);
};

} // ns opengl
