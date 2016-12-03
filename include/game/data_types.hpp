#pragma once
#include <stlw/sized_buffer.hpp>
#include <tuple>

namespace game
{

struct vertex {
  float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
  vertex() = default;
  explicit constexpr vertex(float const xp, float const yp, float const zp, float const wp)
      : x(xp)
      , y(yp)
      , z(zp)
      , w(wp)
  {
  }
};
struct color {
  float r, g, b, a;
  color() = default;
  explicit constexpr color(std::array<float, 4> const &c)
      : color(c[0], c[1], c[2], c[3])
  {
  }
  explicit constexpr color(std::array<float, 3> const &c, float const a)
      : color(c[0], c[1], c[2], a)
  {
  }
  explicit constexpr color(float const rp, float const gp, float const bp, float const ap)
      : r(rp)
      , g(gp)
      , b(bp)
      , a(ap)
  {
  }
};
struct texture_coord {
  float u, v;
  texture_coord() = default;
  explicit constexpr texture_coord(float const up, float const vp)
      : u(up)
      , v(vp)
  {
  }
};

class world_coordinate
{
  vertex v_ = {};

public:
  constexpr world_coordinate() = default;
  explicit constexpr world_coordinate(float const x, float const y, float const z, float const w)
      : v_(x, y, z, w)
  {
  }

#define DEFINE_GET_AND_SET_METHODS(A)                                                              \
  constexpr float A() const { return this->v_.A; }                                                 \
  void set_##A(float const value) { this->v_.A = value; }

  DEFINE_GET_AND_SET_METHODS(x)
  DEFINE_GET_AND_SET_METHODS(y)
  DEFINE_GET_AND_SET_METHODS(z)
  DEFINE_GET_AND_SET_METHODS(w)

  void set(float const xp, float const yp, float const zp, float const wp)
  {
    this->set_x(xp);
    this->set_y(yp);
    this->set_z(zp);
    this->set_w(wp);
  }

  vertex get() const { return this->v_; }
};

} // ns game
