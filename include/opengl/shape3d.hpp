#pragma once
#include <opengl/types.hpp>
#include <opengl/shape.hpp>
#include <array>

namespace opengl
{

template <typename V>
struct cube : public shape {
  static auto constexpr NUM_VERTICES = 8;
  std::array<V, 8> vertices;

private:
  friend class cube_factory;

  explicit constexpr cube(enum draw_mode const dm, struct model const& m, std::array<V, 8> &&v)
      : shape(dm, m)
      , vertices(std::move(v))
  {
  }
};

} // ns opengl
