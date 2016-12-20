#pragma once

namespace engine::gfx
{

enum class shape_type
{
  TRIANGLE = 0,
  RECTANGLE,
  POLYGON,

  // 3d
  CUBE,

  // uhh
  INVALID_SHAPE_TYPE
};

struct height_width
{
  float const height;
  float const width;
};

struct height_width_length
{
  float const height;
  float const width;
  float const length;
};

} // ns engine::gfx
