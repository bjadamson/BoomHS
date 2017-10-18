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

  explicit constexpr cube(GLenum const dm, struct model const& m, std::array<V, 8> &&v)
      : shape(dm, m)
      , vertices(std::move(v))
  {
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

  //int num_vertices() const { return this->vertex_attributes.size(); }
  auto const& vertices() const { return this->object_data_.vertices; }
  auto const& indices() const { return this->object_data_.indices; }
};

} // ns opengl
