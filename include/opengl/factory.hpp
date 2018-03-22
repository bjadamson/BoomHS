#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>

#include <array>
#include <vector>

namespace boomhs
{
struct Obj;
class TileGrid;
} // namespace boomhs

namespace opengl
{
class ShaderProgram;
} // namespace opengl

namespace opengl::factories
{

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
create_arrow_2d(stlw::Logger&, ShaderProgram const&, ArrowCreateParams&&);

DrawInfo
create_arrow(stlw::Logger&, ShaderProgram const&, ArrowCreateParams&&);

DrawInfo
create_tilegrid(stlw::Logger&, ShaderProgram const&, boomhs::TileGrid const&,
                bool const show_yaxis_lines, Color const& color = LOC::RED);

DrawInfo
create_modelnormals(stlw::Logger&, ShaderProgram const&, glm::mat4 const&, boomhs::Obj const&,
                    Color const&);

struct WorldOriginArrows
{
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger&, ShaderProgram&);

struct RectInfo
{
  static constexpr auto NUM_VERTICES = 4;
  float                 width, height;

  // use one, not both (checked in debug builds)
  std::optional<Color>                           color;
  std::optional<std::array<Color, NUM_VERTICES>> colors;

  std::optional<std::array<glm::vec2, NUM_VERTICES>> uvs;
};

struct RectBuffer
{
  std::vector<float>    vertices;
  std::array<GLuint, 6> indices;
};

RectBuffer
create_rectangle(RectInfo const&);

} // namespace opengl::factories

namespace OF = opengl::factories;
