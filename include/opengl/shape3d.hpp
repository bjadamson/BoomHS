#pragma once
#include <opengl/obj.hpp>
#include <opengl/types.hpp>
#include <opengl/shape.hpp>
#include <array>
#include <iostream>

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

template <typename V>
struct mesh : public shape {
  obj const& object_data;
  stlw::sized_buffer<V> vertex_attributes;

  int num_vertices() const { return this->vertex_attributes.size(); }

private:
  friend class mesh_factory;

  explicit constexpr mesh(enum draw_mode const dm, struct model const& m, obj const& object)
      : shape(dm, m)
      , object_data(object)
      , vertex_attributes(this->object_data.indices.size())
  {
  }
};

} // ns opengl
