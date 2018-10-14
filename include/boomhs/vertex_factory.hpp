#pragma once
#include <boomhs/colors.hpp>

#include <opengl/shapes.hpp>
#include <extlibs/glm.hpp>

#include <array>
#include <vector>

using vertices_t = float;
using indices_t  = unsigned int;

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
    using VerticesArray = std::array<float, 18>;
    using PointArray    = std::array<float, 3>;

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
  static RectangleVertices build(float, float, float, float);
  static RectangleVertices build_default();
};

} // namespace boomhs
