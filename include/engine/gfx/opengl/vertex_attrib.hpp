#pragma once
#include <engine/gfx/opengl/gl.hpp>

namespace engine::gfx::opengl
{

struct component_count {
  GLint const vertex;
  GLint const color;
  GLint const uv;

  auto sum() const { return this->vertex + this->color + this->uv; }
};

struct attribute_index {
  GLint const vertex;
  GLint const color;
  GLint const uv;
};

class vertex_attribute_config {
  GLuint const vao_, vbo_;

  // configure this OpenGL VAO attribute with this linear layout
  component_count const ccount_;
  attribute_index const attribute_indexes_;

public:
  vertex_attribute_config(GLuint const vao, GLuint const vbo, component_count const& cc,
      attribute_index const& ai)
    : vao_(vao)
    , vbo_(vbo)
    , ccount_(cc)
    , attribute_indexes_(ai)
  {}

  auto const vao() const { return this->vao_; }
  auto const vbo() const { return this->vbo_; }
  auto num_vertices() const { return this->ccount_.vertex; }
  auto num_colors() const { return this->ccount_.color; }
  auto num_uv() const { return this->ccount_.uv; }
  auto num_components() const { return this->ccount_.sum(); }

  auto const& indexes() const { return this->attribute_indexes_; }
};

template<typename R>
auto
make_vertex_attribute_config(R const& renderable)
{
  // Vertex Attribute positions
  static GLint constexpr VERTEX_INDEX = 0;
  static GLint constexpr COLOR_INDEX = 1;
  static GLint constexpr TEXTURE_COORDINATE_INDEX = 2;

  // component counts (num floats per attribute)
  GLint const cc_vertex = 4; // x, y, z, w
  GLint const cc_color = 4;  // r, g, b, a
  GLint const cc_uv = 2;     // u, v
  component_count const ccount{cc_vertex, cc_color, cc_uv};

  // attribute indexes
  GLint const index_vertex = 0;
  GLint const index_color = 1;
  GLint const index_uv = 2;
  attribute_index const ai{index_vertex, index_color, index_uv};

  return vertex_attribute_config{renderable.vao(), renderable.vbo(), ccount, ai};
}

} // ns engine::gfx::opengl
