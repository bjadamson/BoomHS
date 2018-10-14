#pragma once
#include <boomhs/color.hpp>
#include <common/type_macros.hpp>

#include <array>
#include <optional>
#include <vector>

namespace boomhs
{

using vertices_t = float;
using indices_t  = unsigned int;

using CubeVertices         = std::array<vertices_t, 24>;
using CubeIndices          = std::array<indices_t, 36>;
using CubeWireframeIndices = std::array<indices_t, 24>;

using VerticesArray = std::vector<vertices_t>;
using IndicesArray  = std::vector<indices_t>;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Rectangles
using RectangleUvVertices  = std::array<vertices_t, 30>;

using RectangleIndices     = std::array<indices_t, 6>;
using RectangleLineIndices = std::array<indices_t, 4>;

struct RectBuffer
{
  VerticesArray vertices;
  IndicesArray  indices;

  DEFINE_VECTOR_LIKE_WRAPPER_FNS(vertices);
};

struct RectLineBuffer
{
  std::array<vertices_t, 12> vertices;

  DEFINE_ARRAY_LIKE_WRAPPER_FNS(vertices);
};

class RectangleUvs
{
public:
  using PointArray    = std::array<vertices_t, 2>;
  using VerticesArray = std::array<PointArray, 4>;

private:
  VerticesArray varray_;

public:
  RectangleUvs() = default;
  RectangleUvs(VerticesArray const&);
  RectangleUvs(VerticesArray&&);

  auto const& operator[](size_t const i) const { return varray_[i]; }

  PointArray zero() const;
  PointArray one() const;
  PointArray two() const;
  PointArray three() const;
};

struct RectBuilder
{
  class RectangleColorArray
  {
  public:
    static constexpr int NUM_VERTICES = 4;

  private:
    ColorArray<NUM_VERTICES> data_;

  public:
    DEFINE_ARRAY_LIKE_WRAPPER_FNS(data_);
  };

  //
  // FIELDS
  RectFloat rect;

  // use one of the following rectangle types.
  std::optional<RectangleColorArray> color_array;
  std::optional<Color>               uniform_color;

  std::optional<RectangleUvs>        uvs;

  struct LineRectangleMarker {};
  std::optional<LineRectangleMarker> line;

  RectBuilder(RectFloat const&);

  RectBuffer
  build() const;
};

} // namespace boomhs
