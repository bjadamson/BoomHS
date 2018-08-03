#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shapes.hpp>

#include <array>
#include <optional>
#include <vector>

namespace boomhs
{
struct Obj;
} // namespace boomhs

namespace opengl
{
class ShaderProgram;

struct ArrowCreateParams
{
  Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

} // namespace opengl

namespace opengl::factories
{

// Arrows
///////////////////////////////////////////////////////////////////////////////////////////////////

ArrowVertices
make_arrow_vertices(ArrowCreateParams const&);

// Cubes
///////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
static constexpr CubeIndices CUBE_INDICES = {{
  0, 1, 2,  2, 3, 0, // front
  1, 5, 6,  6, 2, 1, // top
  7, 6, 5,  5, 4, 7, // back
  4, 0, 3,  3, 7, 4, // bottom
  4, 5, 1,  1, 0, 4, // left
  3, 2, 6,  6, 7, 3, // right
}};

static constexpr CubeIndices CUBE_INDICES_LIGHT = {{
  0,  1,  2,    3,  4,  5,
  6,  7,  8,    9,  10, 11,
  12, 13, 14,   15, 16, 17,
  18, 19, 20,   21, 22, 23,
  24, 25, 26,   27, 28, 29,
  30, 31, 32,   33, 34, 35
}};

static constexpr std::array<GLuint, 24> CUBE_WIREFRAME_INDICES = {{
  0, 1, // front
  1, 2,
  2, 3,
  3, 0,

  4, 5, // back
  5, 6,
  6, 7,
  7, 4,

  0, 4, // connect front/back
  1, 5,
  2, 6,
  3, 7
}};

CubeVertices
cube_vertices(glm::vec3 const&, glm::vec3 const&);

// Rectangles
///////////////////////////////////////////////////////////////////////////////////////////////////
struct RectInfo
{
  static constexpr auto NUM_VERTICES = 4;
  float                 width, height;

  // use one, not both (checked in debug builds)
  std::optional<Color>                           color;
  std::optional<std::array<Color, NUM_VERTICES>> colors;

  std::optional<RectangleUvs> uvs;
};

RectBuffer
make_rectangle(RectInfo const&);

static constexpr RectangleIndices RECTANGLE_INDICES = {{
  0, 1, 2, 3, 4, 5
}};

RectangleVertices
rectangle_vertices(float, float, float, float);

RectangleVertices
rectangle_vertices_default();

RectangleUvs
rectangle_uvs(float);

} // namespace opengl::factories

namespace OF = opengl::factories;
