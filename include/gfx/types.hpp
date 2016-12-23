#pragma once

namespace gfx
{

struct vertex_d {
  float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
};
struct color_d {
  float r, g, b, a;
  color_d() = default;
  explicit constexpr color_d(std::array<float, 4> const &c)
      : color_d(c[0], c[1], c[2], c[3])
  {
  }
  explicit constexpr color_d(std::array<float, 3> const &c, float const a)
      : color_d(c[0], c[1], c[2], a)
  {
  }
  explicit constexpr color_d(float const rp, float const gp, float const bp, float const ap)
      : r(rp)
      , g(gp)
      , b(bp)
      , a(ap)
  {
  }
};
struct uv_d {
  float u, v;
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

} // ns gfx
