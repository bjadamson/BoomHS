#pragma once
#include <array>
#include <iostream>
#include <opengl/obj.hpp>
#include <opengl/types.hpp>
#include <opengl/shape.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

template <typename V, std::size_t N>
class cube : public shape {
  std::array<float, N> vertices_;

  friend class cube_factory;
public:
  static auto constexpr NUM_VERTICES = 8;

  explicit constexpr cube(GLenum const dm, struct model const& m, std::array<float, N> &&v)
      : shape(dm, m)
      , vertices_(MOVE(v))
  {
  }

  auto const num_vertices() const { return NUM_VERTICES; }
  auto const& vertices() const { return this->vertices_; }
  auto const& indices() const
  {
    // clang-format off
    static constexpr auto INDICES = std::array<GLuint, 14> {
      3, 2, 6, 7, 4, 2, 0,
      3, 1, 6, 5, 4, 1, 0
    };
    // clang-format on
    return INDICES;
  }
};

template <typename V>
class mesh : public shape {
  obj const& object_data_;

public:
  friend class mesh_factory;

  explicit constexpr mesh(GLenum const dm, struct model const& m, obj const& object)
      : shape(dm, m)
      , object_data_(object)
  {
  }

  auto num_vertices() const { return this->object_data_.num_vertices; }
  auto const& vertices() const { return this->object_data_.vertices; }
  auto const& indices() const { return this->object_data_.indices; }
};

} // ns opengl
