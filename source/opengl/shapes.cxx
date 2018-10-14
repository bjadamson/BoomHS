#include <opengl/shapes.hpp>
#include <common/algorithm.hpp>

namespace opengl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// RectangleUvs
RectangleUvs::RectangleUvs()
{
  common::memzero(varray_.data(), sizeof(float) * varray_.size());
}

RectangleUvs::RectangleUvs(VerticesArray const& va)
  : varray_(va)
{
}

RectangleUvs::RectangleUvs(VerticesArray &&va)
  : varray_(MOVE(va))
{
}

RectangleUvs::PointArray
RectangleUvs::zero() const
{
  auto const& v = varray_[0];
  return PointArray{common::make_array<float>(v[0], 1.0f - v[1])};
}

RectangleUvs::PointArray
RectangleUvs::one() const
{
  auto const& v = varray_[1];
  return PointArray{common::make_array<float>(v[0], 1.0f - v[1])};
}

RectangleUvs::PointArray
RectangleUvs::two() const
{
  auto const& v = varray_[2];
  return PointArray{common::make_array<float>(v[0], 1.0f - v[1])};
}

RectangleUvs::PointArray
RectangleUvs::three() const
{
  auto const& v = varray_[3];
  return PointArray{common::make_array<float>(v[0], 1.0f - v[1])};
}

} // namespace opengl
