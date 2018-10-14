#pragma once
#include <boomhs/colors.hpp>
#include <boomhs/math.hpp>
#include <boomhs/vertex_factory.hpp>

#include <opengl/draw_info.hpp>
#include <boomhs/shapes.hpp>

#include <array>
#include <optional>
#include <vector>

namespace boomhs
{
struct Obj;
} // namespace boomhs

namespace opengl
{

struct RectangleFactory
{
  RectangleFactory() = delete;

  static boomhs::RectangleUvVertices from_vertices_and_uvs(boomhs::VertexFactory::RectangleVertices const&,
                                                   boomhs::UvFactory::RectangleUvs const&);
};

} // namespace opengl

namespace opengl::factories
{

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
  std::optional<RectangleColorArray>             color_array;
  std::optional<boomhs::Color>                   uniform_color;

  std::optional<boomhs::UvFactory::RectangleUvs> uvs;

  RectBuilder(boomhs::RectFloat const&);

  boomhs::RectBuffer
  build() const;
};

boomhs::RectBuffer
make_rectangle(RectBuilder const&);

boomhs::RectLineBuffer
make_line_rectangle(boomhs::RectFloat const&);

} // namespace opengl::factories

namespace OF = opengl::factories;
