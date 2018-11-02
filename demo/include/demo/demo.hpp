#pragma once
#include <boomhs/color.hpp>
#include <boomhs/math.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/random.hpp>
#include <boomhs/transform.hpp>

#include <opengl/gpu.hpp>
#include <opengl/vertex_attribute.hpp>

#include <vector>

namespace demo
{

enum ScreenSector
{
  LEFT_TOP = 0,
  RIGHT_TOP,
  LEFT_BOTTOM,
  RIGHT_BOTTOM,
  MAX
};

struct MouseCursorInfo
{
  ScreenSector sector;
  boomhs::MouseClickPositions click_positions;
};


inline auto
make_perspective_rect_gpuhandle(common::Logger& logger, boomhs::RectFloat const& rect,
                                opengl::VertexAttribute const& va)
{
  using namespace boomhs;

  auto buffer = RectBuilder{rect}.build();
  return OG::copy_rectangle(logger, va, buffer);
}

inline auto
make_perspective_rect(boomhs::Viewport const& viewport, boomhs::RNG& rng)
{
  auto const gen = [&rng](auto const low, auto const high) { return rng.gen_float_range(low, high); };

  auto const left  = gen(viewport.left(), viewport.right());
  auto const right = gen(left, viewport.right());

  auto const top    = gen(viewport.top(), viewport.bottom());
  auto const bottom = gen(top, viewport.bottom());

  return boomhs::RectFloat{left, top, right, bottom};
};

} // namespace demo
