#pragma once
#include <boomhs/colors.hpp>
#include <boomhs/math.hpp>
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
  boomhs::Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

struct ArrowFactory
{
  ArrowFactory() = delete;

  static ArrowVertices create_vertices(ArrowCreateParams const&);

  static constexpr std::array<GLuint, 6> INDICES = {{0, 1, 2, 3, 4, 5}};
};

struct LineCreateParams
{
  glm::vec3 start;
  glm::vec3 end;
};

struct LineFactory
{
  LineFactory() = delete;

  static LineVertices create_vertices(LineCreateParams const&);

  static constexpr std::array<GLuint, 2> INDICES = {{0, 1}};
};

struct RectangleFactory
{
  RectangleFactory() = delete;

  static RectangleUvVertices from_vertices_and_uvs(RectangleVertices const&, RectangleUvs const&);
};

struct GridVerticesIndices
{
  std::vector<float>  vertices;
  std::vector<GLuint> indices;
};
struct GridFactory
{
  GridFactory() = delete;

  static GridVerticesIndices create_grid(glm::vec2 const&, bool, boomhs::Color const&);
};

} // namespace opengl

namespace opengl::factories
{

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
class RectangleColorArray
{
public:
  static constexpr int NUM_VERTICES = 4;

private:
  boomhs::ColorArray<NUM_VERTICES> data_;

public:
  DEFINE_ARRAY_LIKE_WRAPPER_FNS(data_);
};

struct RectBuilder
{
  boomhs::RectFloat                  rect;

  // use one of the following rectangle types.
  std::optional<RectangleColorArray> color_array;
  std::optional<boomhs::Color>       uniform_color;

  std::optional<RectangleUvs>        uvs;

  RectBuilder(boomhs::RectFloat const&);

  RectBuffer
  build() const;
};

RectBuffer
make_rectangle(RectBuilder const&);

RectLineBuffer
make_line_rectangle(boomhs::RectFloat const&);

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
