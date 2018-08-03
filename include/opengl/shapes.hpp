#pragma once
#include <array>
#include <extlibs/glew.hpp>
#include <extlibs/glm.hpp>
#include <vector>

namespace opengl
{

struct CubeMinMax
{
  glm::vec3 min, max;
};

using ArrowVertices = std::array<float, 42>;

using CubeVertices = std::array<float, 24>;
using CubeIndices  = std::array<GLuint, 36>;

using RectangleIndices = std::array<GLuint, 6>;

struct RectBuffer
{
  std::vector<float> vertices;
  RectangleIndices   indices;
};

class RectangleVertices
{
  using VerticesArray = std::array<float, 18>;
  using PointArray    = std::array<float, 3>;

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
};

using PointArray = std::array<float, 2>;
class RectangleUvs
{
  using VerticesArray = std::array<PointArray, 4>;
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

} // namespace opengl
