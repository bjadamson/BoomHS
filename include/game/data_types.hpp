#pragma once
#include <tuple>
#include <stlw/sized_buffer.hpp>

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

struct polygon : public shape {
  stlw::sized_buffer<vertex> vertices;
  auto num_vertices() const { return this->vertices.length(); }
private:
  friend class shape_factory;
  explicit polygon(world_coordinate const& wc, int const num_vertices)
    : shape(wc)
    , vertices(num_vertices)
  {
  }

public:
  friend struct shape_factory;
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

  static auto make_polygon(world_coordinate const& wc, int const num_vertices)
  {
    constexpr float width = 0.025;
    constexpr float radius = width;
    auto const V = num_vertices;
    auto const C = V; // Assume for now #colors == #vertices
    auto const E = V + 1; // num_edges

    auto const cosfn = [&radius, &wc, &E](auto const a) {
      auto const pos = radius * static_cast<float>(std::cos(2*M_PI*a/E));
      return wc.x() + pos;
    };
    auto const sinfn = [&radius, &wc, &E](auto const a) {
      auto const pos = radius * static_cast<float>(std::sin(2*M_PI*a/E));
      return wc.y() + pos;
    };

    polygon p{wc, num_vertices};
    for (auto i{0}, j{0}; i < V; ++i) {
      p.vertices[i] = vertex{cosfn(i), sinfn(i), wc.z(), wc.w()};
    }
    return p;
  }
};

} // ns game
