#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/renderer.hpp>

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
  static auto constexpr RED_TRIANGLE_VERTEX_COLOR_INDEX = 1;
  static auto constexpr RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX = 2;

public:
  static stlw::result<renderer, std::string> make_renderer();
};

} // opengl
} // ns gfx
} // ns engine
