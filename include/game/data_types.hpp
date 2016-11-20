#pragma once
#include <tuple>

namespace game
{

struct vertex
{
  float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
  vertex() = default;
  explicit constexpr vertex(float const xp, float const yp, float const zp, float const wp)
    : x(xp)
    , y(yp)
    , z(zp)
    , w(wp)
    {}
};
struct color {
  float r, g, b, a;
  explicit constexpr color(float const rp, float const gp, float const bp, float const ap)
    : r(rp)
    , g(gp)
    , b(bp)
    , a(ap)
    {}
  static constexpr std::array<float, 4> DEFAULT_TEXTURE() { return {1.0f, 0.0f, 0.0f, 0.0f}; };
};
struct texture_coord {
  float u, v;
  explicit constexpr texture_coord(float const up, float const vp)
    : u(up)
    , v(vp)
    {}
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

  void
  set(float const xp, float const yp, float const zp, float const wp)
  {
    this->set_x(xp);
    this->set_y(yp);
    this->set_z(zp);
    this->set_w(wp);
  }

  vertex
  get() const
  {
    return this->v_;
  }
};

struct shape {
  world_coordinate coord_;
protected:
  explicit constexpr shape(world_coordinate const& wc) : coord_(wc) {}
public:
  auto constexpr const& wc() const { return this->coord_; }
};

struct triangle : public shape {
  vertex bottom_left, bottom_right, top_middle;
private:
  friend class shape_factory;
  explicit constexpr triangle(world_coordinate const& wc) : shape(wc) {}
};

struct rectangle : public shape {
  vertex bottom_left, bottom_right, top_right, top_left;
private:
  friend class shape_factory;
  explicit constexpr rectangle(world_coordinate const& wc) : shape(wc) {}
};

struct shape_factory {
  shape_factory() = delete;

  static constexpr auto make_triangle(world_coordinate const& wc)
  {
    constexpr float radius = 0.5;

    // clang-format off
    triangle t{wc};
    t.bottom_left  = vertex{wc.x() - radius, wc.y() - radius, wc.z(), wc.w()};
    t.bottom_right = vertex{wc.x() + radius, wc.y() - radius, wc.z(), wc.w()};
    t.top_middle   = vertex{wc.x()         , wc.y() + radius, wc.z(), wc.w()};

    // clang-format on
    return t;
  }

  static auto make_rectangle(world_coordinate const& wc)
  {
    constexpr float width = 0.25;
    constexpr float height = 0.39;

    // clang-format off
    rectangle r{wc};
    r.bottom_left = vertex{wc.x() - width, wc.y() - height, wc.z(), wc.w()};
    r.bottom_right = vertex{wc.x() + width, wc.y() - height, wc.z(), wc.w()};
    r.top_right = vertex{wc.x() + width, wc.y() + height, wc.z(), wc.w()};
    r.top_left = vertex{wc.x() - width, wc.y() + height, wc.z(), wc.w()};

    // clang-format on
    return r;
  }
};

} // ns game
