#pragma once
#include <boomhs/colors.hpp>

#include <boomhs/shapes.hpp>
#include <extlibs/glm.hpp>

#include <array>
#include <vector>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Arrow
struct ArrowTemplate
{
  Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

struct LineTemplate
{
  glm::vec3 start;
  glm::vec3 end;
};

struct GridTemplate
{
  glm::vec3    dimensions;
  Color const& color;
};

class VertexFactory
{
  VertexFactory() = delete;
public:

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Arrow
  using ArrowVertices                         = std::array<vertices_t, 42>;
  using ArrowIndices                          = std::array<indices_t,  6>;
  static constexpr ArrowIndices ARROW_INDICES = {{0, 1, 2, 3, 4, 5}};
  static ArrowVertices build(ArrowTemplate const&);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Line
  using LineVertices                        = std::array<vertices_t, 6>;
  using LineIndices                         = std::array<indices_t,  2>;
  static constexpr LineIndices LINE_INDICES = {{0, 1}};
  static LineVertices build(LineTemplate const&);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Grid
  using GridVertices                        = std::vector<vertices_t>;
  using GridIndices                         = std::vector<indices_t>;

  struct GridVerticesIndices
  {
    GridVertices vertices;
    GridIndices  indices;
  };
  static GridVerticesIndices build(GridTemplate const&);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Rectangle
  class RectangleVertices
  {
  public:
    using VerticesArray = std::array<vertices_t, 18>;
    using PointArray    = std::array<vertices_t, 3>;

  private:
    VerticesArray varray_;

  public:
    RectangleVertices();
    RectangleVertices(VerticesArray const&);
    RectangleVertices(VerticesArray&&);

    auto const& operator[](size_t const i) const { return varray_[i]; }

    PointArray zero() const;
    PointArray one() const;
    PointArray two() const;
    PointArray three() const;
    PointArray four() const;
    PointArray five() const;

    auto const& array() const { return varray_; }
  };
  static RectangleVertices build(vertices_t, vertices_t, vertices_t, vertices_t);
  static RectangleVertices build_default();

  static constexpr RectangleIndices RECTANGLE_DEFAULT_INDICES = {{
    0, 1, 2, 3, 4, 5
  }};

  static constexpr RectangleLineIndices RECTANGLE_LINE_INDICES = {{
    0, 1, 2, 3
  }};

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Cube

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

  static constexpr CubeWireframeIndices CUBE_WIREFRAME_INDICES = {{
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
  // clang-format on

  static CubeVertices build_cube(glm::vec3 const&, glm::vec3 const&);
};

class UvFactory
{
  UvFactory() = delete;
public:
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Rectangle
  class RectangleUvs
  {
  public:
    using PointArray    = std::array<vertices_t, 2>;
    using VerticesArray = std::array<PointArray, 4>;

  private:
    VerticesArray varray_;

  public:
    RectangleUvs();
    RectangleUvs(VerticesArray const&);
    RectangleUvs(VerticesArray&&);

    auto const& operator[](size_t const i) const { return varray_[i]; }

    PointArray zero() const;
    PointArray one() const;
    PointArray two() const;
    PointArray three() const;
  };

  static RectangleUvs build_rectangle(float);
};


} // namespace boomhs
