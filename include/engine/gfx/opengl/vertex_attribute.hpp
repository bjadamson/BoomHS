#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/glsl.hpp>

namespace engine::gfx::opengl
{

struct attribute_info
{
  GLint const global_index;
  GLint const num_floats;
};

struct attribute_list {
  attribute_info const vertex;
  attribute_info const color;
  attribute_info const uv;

  auto num_floats() const
  {
    return this->vertex.num_floats + this->color.num_floats + this->uv.num_floats;
  }
};

class vertex_attribute
{
  GLuint const vao_, vbo_;

  // configure this OpenGL VAO attribute with this linear layout
  attribute_list const list_;

public:
  vertex_attribute(GLuint const vao, GLuint const vbo, attribute_list const& list)
      : vao_(vao)
      , vbo_(vbo)
      , list_(list)
  {
  }

  auto const vao() const { return this->vao_; }
  auto const vbo() const { return this->vbo_; }
  auto vertices() const { return this->list_.vertex; }
  auto colors() const { return this->list_.color; }
  auto uv() const { return this->list_.uv; }

  auto num_floats() const { return this->list_.num_floats(); }
};

auto
make_vertex_attribute(opengl_context const &ctx)
{
  // num floats per attribute
  GLint constexpr nf_vertex = 4; // x, y, z, w
  GLint constexpr nf_color = 4;  // r, g, b, a
  GLint constexpr nf_uv = 2;     // u, v

  // clang-format off
  static attribute_info constexpr vertex_info{VERTEX_ATTRIBUTE_INDEX_OF_POSITION, nf_vertex};
  static attribute_info constexpr color_info {VERTEX_ATTRIBUTE_INDEX_OF_COLOR, nf_color};
  static attribute_info constexpr uv_info    {VERTEX_ATTRIBUTE_INDEX_OF_UV, nf_uv};
  // clang-format on

  // attribute indexes
  attribute_list constexpr list{vertex_info, color_info, uv_info};
  return vertex_attribute{ctx.vao(), ctx.vbo(), std::move(list)};
}

namespace global
{

template <typename L>
auto
set_vertex_attributes(L &logger, vertex_attribute const &va)
{
  vao_bind(va.vao());
  ON_SCOPE_EXIT([]() { vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, va.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  struct skip_context {
    GLsizei const total_component_count;
    GLsizei components_skipped = 0;

    skip_context(GLsizei const c)
        : total_component_count(c)
    {
    }
  };

  skip_context sc{va.num_floats()};
  auto const set_attrib_pointer = [&logger, &sc](auto const& attribute_info)
  {
    auto const attribute_index = attribute_info.global_index;
    auto const component_count = attribute_info.num_floats;

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

    auto const s = fmt::format("%-10d %-10d %-10d %-10s %-10d %-10d %-10d %-10d\n", attribute_index,
                               enabled, component_count, "float", DONT_NORMALIZE_THE_DATA,
                               stride_in_bytes, offset_in_bytes, sc.components_skipped);
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
  set_attrib_pointer(va.vertices());
  set_attrib_pointer(va.colors());
  set_attrib_pointer(va.uv());
};

} // ns global

} // ns engine::gfx::opengl
