#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <opengl/glew.hpp>

namespace opengl
{

struct vertex_d {
  float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
};
struct normal_d {
  float nx = 1.f, ny = 1.f, nz = 1.f;
};
struct color_d {
  float r, g, b;
  float a = 1.0f;

  color_d() = default;
  constexpr color_d(float const rp, float const gp, float const bp, float const ap)
      : r(rp)
      , g(gp)
      , b(bp)
      , a(ap)
  {
  }
  constexpr color_d(float const rr, float const gg, float const bb)
    : r(rr)
    , g(gg)
    , b(bb)
  {
  }
  constexpr color_d(std::array<float, 4> const &c)
      : r(c[0])
      , g(c[1])
      , b(c[2])
      , a(c[3])
  {
  }
  explicit constexpr color_d(std::array<float, 3> const& c)
    : color_d(c[0], c[1], c[2])
  {
  }
  constexpr color_d(std::array<float, 3> const &c, float const a)
      : color_d(c[0], c[1], c[2], a)
  {
  }
};
struct uv_d {
  float u, v;
};

// tags
struct vertex_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 4; };
struct normal_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 3; };
struct color_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 4; };
struct uv_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 2; };
struct wireframe_t { static constexpr std::size_t NUM_FLOATS_PER_VERTEX = 0; };

struct Model
{
  glm::vec3 translation{0.0f, 0.0f, 0.0f};
  glm::quat rotation;
  glm::vec3 scale = glm::vec3{1.0f, 1.0f, 1.0f};
};

struct height_width
{
  float const height;
  float const width;
};

struct width_height_length
{
  float const width;
  float const height;
  float const length;
};

} // ns opengl
