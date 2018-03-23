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

// Arrows
///////////////////////////////////////////////////////////////////////////////////////////////////
using ArrayVertices = std::array<float, 48>;

struct ArrowCreateParams
{
  Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

ArrayVertices
make_arrow_vertices(ArrowCreateParams const&);

// Cubes
///////////////////////////////////////////////////////////////////////////////////////////////////
using CubeVertices = std::array<float, 32>;
using CubeIndices = std::array<GLuint, 36>;

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
  0, 1, 2, 3, 4, 5, 6,
  7, 8, 9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 34, 35
}};

CubeVertices
cube_vertices();

// Rectangles
///////////////////////////////////////////////////////////////////////////////////////////////////
using RectangleVertices = std::array<float, 16>;
using RectangleIndices = std::array<GLuint, 6>;
using RectangleNormals = std::array<float, 12>;
using RectangleUvs = std::array<float, 8>;

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
  std::vector<float> vertices;
  RectangleIndices   indices;
};

RectBuffer
make_rectangle(RectInfo const&);


static constexpr RectangleIndices RECTANGLE_INDICES = {{
  0, 1, 2,
  2, 3, 0
}};

RectangleVertices
rectangle_vertices();

RectangleNormals
rectangle_normals(RectangleVertices const&);

constexpr RectangleUvs
rectangle_uvs(float const max)
{
  return stlw::make_array<float>(
      0.0f, 0.0f,
      max, 0.0f,
      max, max,
      0.0f, max
      );
}

// Terrain
///////////////////////////////////////////////////////////////////////////////////////////////////
static constexpr RectangleIndices TERRAIN_INDICES = RECTANGLE_INDICES;

RectangleVertices
terrain_vertices();

} // namespace opengl::factories

namespace OF = opengl::factories;
