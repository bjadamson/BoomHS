#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/texture.hpp>

#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>

#include <boomhs/tilemap.hpp>
#include <boomhs/types.hpp>

#include <boost/optional.hpp>
#include <array>
#include <cmath>

namespace opengl
{
class ShaderProgram;
struct obj;
} // ns opengl

namespace opengl::cube_factory
{
  // clang-format off
  static constexpr std::array<GLuint, 36> INDICES = {{
    0, 1, 2,  2, 3, 0, // front
    1, 5, 6,  6, 2, 1, // top
    7, 6, 5,  5, 4, 7, // back
    4, 0, 3,  3, 7, 4, // bottom
    4, 5, 1,  1, 0, 4, // left
    3, 2, 6,  6, 7, 3, // right
  }};

  static constexpr std::array<GLuint, 36> INDICES_LIGHT = {{
    0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13,
    14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27,
    28, 29, 30, 31, 32, 33, 34, 35
  }};

  /*
  static constexpr std::array<GLint, (36 / 2)> NORMALS = {{
    0, 1, -1, // front
    0, 1, 0,  // top
    0, 0, 1,  // back
    0, -1, 0, // bottom
    -1, 0, 0, // left
    1, 0, 0   // right
  }};
  */
  // clang-format on

} // ns opengl::cube_factory

namespace opengl::factories
{

DrawInfo
copy_colorcube_gpu(stlw::Logger &, ShaderProgram const&, Color const&);

inline DrawInfo
copy_colorcube_gpu(stlw::Logger &logger, ShaderProgram const& sp, glm::vec3 const& c)
{
  return copy_colorcube_gpu(logger, sp, Color{c.x, c.y, c.z, 1.0f});
}

DrawInfo
copy_vertexonlycube_gpu(stlw::Logger &, ShaderProgram const&);

DrawInfo
copy_normalcolorcube_gpu(stlw::Logger &, ShaderProgram const&, Color const&);

DrawInfo
copy_texturecube_gpu(stlw::Logger &, ShaderProgram const&, TextureInfo const&);

DrawInfo
copy_cube_14indices_gpu(stlw::Logger &, ShaderProgram const&, boost::optional<TextureInfo> const&);

struct ArrowCreateParams
{
  Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

struct ArrowEndpoints
{
  glm::vec3 p1;
  glm::vec3 p2;
};

DrawInfo
create_arrow_2d(stlw::Logger &, ShaderProgram const&, ArrowCreateParams &&);

DrawInfo
create_arrow(stlw::Logger &, ShaderProgram const&, ArrowCreateParams &&);

DrawInfo
create_tilegrid(stlw::Logger &, ShaderProgram const&, boomhs::TileMap const&,
    bool const show_yaxis_lines, Color const& color = LOC::RED);

struct WorldOriginArrows {
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger &, ShaderProgram &, glm::vec3 const&);

WorldOriginArrows
create_world_axis_arrows(stlw::Logger &logger, ShaderProgram &);

DrawInfo
copy_gpu(stlw::Logger &, GLenum const, ShaderProgram &, obj const&, boost::optional<TextureInfo> const&);

} // ns opengl::factories
