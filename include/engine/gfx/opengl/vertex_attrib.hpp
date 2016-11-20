#pragma once
#include <engine/gfx/opengl/gl.hpp>
#include <engine/gfx/opengl/context.hpp>

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

auto
make_vertex_attribute_config(render_context const& rc)
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

  return vertex_attribute_config{rc.vao(), rc.vbo(), ccount, ai};
}

namespace global {

template<typename L>
auto
set_vertex_attributes(L &logger, vertex_attribute_config const& config)
{
  vao_bind(config.vao());
  ON_SCOPE_EXIT([]() { vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, config.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  struct skip_context {
    GLsizei const total_component_count;
    GLsizei components_skipped = 0;

    skip_context(GLsizei const c)
        : total_component_count(c)
    {
    }
  };

  skip_context sc{config.num_components()};
  auto const set_attrib_pointer = [&logger, &sc](auto const attribute_index, auto const component_count) {
    // enable vertex attibute arrays
    glEnableVertexAttribArray(attribute_index);

    static auto constexpr DONT_NORMALIZE_THE_DATA = GL_FALSE;

    // clang-format off
    auto const offset_in_bytes = sc.components_skipped * sizeof(GL_FLOAT);
    auto const stride_in_bytes = sc.total_component_count * sizeof(GL_FLOAT);
    glVertexAttribPointer(
        attribute_index,                             // global index id
        component_count,                             // number of components per attribute
        GL_FLOAT,                                    // data-type of the components
        DONT_NORMALIZE_THE_DATA,                     // don't normalize our data
        stride_in_bytes,                             // byte-offset between consecutive vertex attributes
        reinterpret_cast<GLvoid*>(offset_in_bytes)); // offset from beginning of buffer
    // clang-format on
    sc.components_skipped += component_count;

    GLint enabled = 0;
    glGetVertexAttribiv(attribute_index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);

    auto const s =
        fmt::format("%-10d %-10d %-10d %-10s %-10d %-10d %-10d %-10d\n", attribute_index, enabled,
                    component_count, "float", DONT_NORMALIZE_THE_DATA, stride_in_bytes,
                    offset_in_bytes, sc.components_skipped);
    logger.trace(s);
  };

  {
    int max_attribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
    auto const fmt = fmt::format("maximum number of vertex attributes: '%d'\n", max_attribs);
    logger.trace(fmt);
  }
  {
    auto const fmt = fmt::format("%-10s %-10s %-10s %-10s %-10s %-10s %-10s\n", "index:", "enabled",
                                "size", "type", "normalized", "stride", "pointer", "num skipped");
    logger.trace(fmt);
  }
  set_attrib_pointer(config.indexes().vertex, config.num_vertices());
  set_attrib_pointer(config.indexes().color, config.num_colors());
  set_attrib_pointer(config.indexes().uv, config.num_uv());
};

} // ns global

} // ns engine::gfx::opengl
