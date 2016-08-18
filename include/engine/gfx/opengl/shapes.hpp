#pragma once
#include <array>
#include <engine/gfx/opengl_glew.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{
struct point {
  GLfloat const x, y, z, w;
  explicit constexpr point(GLfloat const xp, GLfloat const yp, GLfloat const zp, GLfloat const wp) :
    x(xp),
    y(yp),
    z(zp),
    w(wp) {}
};
struct color {
  GLfloat const r, g, b, a;
  explicit constexpr color(GLfloat const rp, GLfloat const gp, GLfloat const bp, GLfloat const ap) :
    r(rp),
    g(gp),
    b(bp),
    a(ap) {}
};
struct triangle {
  std::array<point,3> const points;
  std::array<color,3> const colors;
  explicit constexpr triangle(std::array<point,3> const& p, std::array<color,3> const c) :
    points(p), colors(c)
  {
  }
};
struct rectangle {
  std::array<point, 4> const points;
  std::array<point, 4> const colors;
  explicit constexpr rectangle(std::array<point,4> const& p, std::array<color,4> const c) :
    points(p), colors(c)
  {
  }
};

} // ns opengl
} // ns gfx
} // ns engine
