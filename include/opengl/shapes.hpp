#pragma once
#include <array>
#include <extlibs/glew.hpp>
#include <extlibs/glm.hpp>
#include <vector>

namespace opengl
{

using CubeVertices = std::array<float, 24>;
using CubeIndices  = std::array<GLuint, 36>;

using RectangleUvVertices = std::array<float, 30>;

using RectangleIndices     = std::array<GLuint, 6>;
using RectangleLineIndices = std::array<GLuint, 4>;

struct RectBuffer
{
  std::vector<float> vertices;
  RectangleIndices   indices;
};

struct RectLineBuffer
{
  std::vector<float>   vertices;
  RectangleLineIndices indices;
};

class RectangleUvs
{
public:
  using PointArray    = std::array<float, 2>;
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

} // namespace opengl
