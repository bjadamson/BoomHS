#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/red_triangle.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

class factory
{
  NO_COPY_AND_NO_MOVE(factory);

  static auto constexpr RED_TRIANGLE_VERTEX_POSITION_INDEX = 0;
  static auto constexpr RED_TRIANGLE_VERTEX_COLOR_INDEX = 0;

public:
  static stlw::result<red_triangle, std::string> make_red_triangle_program();
};

} // opengl
} // ns gfx
} // ns engine
