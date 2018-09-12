#pragma once
#include <array>
#include <extlibs/glew.hpp>
#include <extlibs/glm.hpp>
#include <vector>

namespace opengl
{

using ArrowVertices = std::array<float, 42>;
using ArrowIndices  = std::array<GLuint, 6>;

using LineVertices = std::array<float, 6>;
using LineIndices  = std::array<float, 2>;

using CubeVertices = std::array<float, 24>;
using CubeIndices  = std::array<GLuint, 36>;

using RectangleUvVertices = std::array<float, 30>;

using RectangleIndices = std::array<GLuint, 6>;

struct RectBuffer
{
  std::vector<float> vertices;
  RectangleIndices   indices;
};

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
