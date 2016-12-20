#pragma once

namespace engine::gfx
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

struct model
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};
};

enum class shape_type
{
  TRIANGLE = 0,
  RECTANGLE,
  POLYGON,

  // 3d
  CUBE,

  // uhh
  INVALID_SHAPE_TYPE
};

struct height_width
{
  float const height;
  float const width;
};

struct height_width_length
{
  float const height;
  float const width;
  float const length;
};

} // ns engine::gfx
