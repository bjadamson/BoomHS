#pragma once
#include <boomhs/shape.hpp>
#include <boomhs/vertex_factory.hpp>

namespace boomhs
{

// Interleave the vertices and the UVs of the Rectangles together.
constexpr RectangleUvVertices
vertex_interleave(VertexFactory::RectangleVertices const& v, RectangleUvs const& uv)
{
  return common::concat(
    v.zero(), uv.zero(),
    v.one(),  uv.one(),
    v.two(),  uv.two(),

    v.three(), uv.two(),
    v.four(),  uv.three(),
    v.five(),  uv.zero()
  );
}

} // namespace boomhs
