#include <opengl/shapes.hpp>
#include <stlw/algorithm.hpp>

namespace opengl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Rectangles
RectangleVertices::RectangleVertices()
{
  stlw::memzero(varray_.data(), sizeof(float) * varray_.size());
}

RectangleVertices::RectangleVertices(VerticesArray const& va)
  : varray_(va)
{
}

RectangleVertices::RectangleVertices(VerticesArray &&va)
  : varray_(MOVE(va))
{
}

RectangleVertices::PointArray
RectangleVertices::zero() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[0], v[1], v[2]);
}

RectangleVertices::PointArray
RectangleVertices::one() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[3], v[4], v[5]);
}

RectangleVertices::PointArray
RectangleVertices::two() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[6], v[7], v[8]);
}

RectangleVertices::PointArray
RectangleVertices::three() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[9], v[10], v[11]);
}

RectangleVertices::PointArray
RectangleVertices::four() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[12], v[13], v[14]);
}

RectangleVertices::PointArray
RectangleVertices::five() const
{
  auto const& v = varray_;
  return stlw::make_array<float>(v[15], v[16], v[17]);
}

////////////////////////
RectangleUvs::RectangleUvs()
{
  stlw::memzero(varray_.data(), sizeof(float) * varray_.size());
}

RectangleUvs::RectangleUvs(VerticesArray const& va)
  : varray_(va)
{
}

RectangleUvs::RectangleUvs(VerticesArray &&va)
  : varray_(MOVE(va))
{
}

PointArray
RectangleUvs::zero() const
{
  auto const& v = varray_[0];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

PointArray
RectangleUvs::one() const
{
  auto const& v = varray_[1];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

PointArray
RectangleUvs::two() const
{
  auto const& v = varray_[2];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

PointArray
RectangleUvs::three() const
{
  auto const& v = varray_[3];
  return PointArray{stlw::make_array<float>(v[0], 1.0f - v[1])};
}

} // namespace opengl
