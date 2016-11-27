#pragma once
#include <boost/optional.hpp>
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/glsl.hpp>
#include <stlw/type_ctors.hpp>

namespace engine::gfx::opengl
{

struct attribute_info {
  GLint const global_index;
  GLint const num_floats;
  char const* name;

  explicit constexpr attribute_info(GLint const i, GLint const nf, char const* n)
      : global_index(i)
      , num_floats(nf)
      , name(n)
  {
  }
};

using optional_attribute = boost::optional<attribute_info>;

struct attribute_list {
  optional_attribute const vertex;
  optional_attribute const color;
  optional_attribute const uv;

  auto num_floats() const
  {
    auto const add = [](auto const &opt) { return opt ? opt->num_floats : 0; };
    return add(this->vertex) + add(this->color) + add(this->uv);
  }

  static constexpr auto A_POSITION = "a_position";
  static constexpr auto A_COLOR = "a_color";
  static constexpr auto A_UV = "a_uv";

  static constexpr auto table()
  {
    // clang-format off
    // num fields per attribute
    GLint constexpr num_fields_vertex = 4; // x, y, z, w
    GLint constexpr num_fields_color = 4;  // r, g, b, a
    GLint constexpr num_fields_uv = 2;     // u, v

    attribute_info constexpr vertex_info{VERTEX_ATTRIBUTE_INDEX_OF_POSITION, num_fields_vertex, A_POSITION};
    attribute_info constexpr color_info {VERTEX_ATTRIBUTE_INDEX_OF_COLOR,    num_fields_color,  A_COLOR};
    attribute_info constexpr uv_info    {VERTEX_ATTRIBUTE_INDEX_OF_UV,       num_fields_uv,     A_UV};
    return stlw::make_array<3, attribute_info>(vertex_info, color_info, uv_info);
    // clang-format on
  }
};

class vertex_attribute
{
  GLuint const vao_, vbo_;

  // configure this OpenGL VAO attribute with this linear layout
  attribute_list const list_;

public:
  vertex_attribute(GLuint const vao, GLuint const vbo, attribute_list const &list)
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

inline vertex_attribute
make_color_uv_vertex_attribute(opengl_context const &ctx)
{
  // attribute indexes
  static constexpr auto table = attribute_list::table();
  static constexpr auto V_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_POSITION;
  static constexpr auto C_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_COLOR;
  static constexpr auto UV_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_UV;

  attribute_list list{table[V_INDEX], table[C_INDEX], table[UV_INDEX]};
  return vertex_attribute{ctx.vao(), ctx.vbo(), std::move(list)};
}

namespace impl
{

struct skip_context {
  GLsizei const total_component_count;
  GLsizei components_skipped = 0;

  explicit skip_context(GLsizei const c)
      : total_component_count(c)
  {
  }
};

/*
template<typename L>
set_attrib_pointer(L &logger, attribute_info const& attrib_info, skip_context &sc)
{
  // enable vertex attibute arrays
  glEnableVertexAttribArray(attribute_index);

  static auto constexpr DONT_NORMALIZE_THE_DATA = GL_FALSE;

  // clang-format off
  auto const offset_in_bytes = sc.components_skipped * sizeof(GL_FLOAT);
  auto const stride_in_bytes = sc.total_component_count * sizeof(GL_FLOAT);

  glVertexAttribPointer(
      attrib_info.global_index,                    // global index id
      attrib_info.num_components,                  // number of components per attribute
      attrib_info.type,                            // data-type of the components
      DONT_NORMALIZE_THE_DATA,                     // don't normalize our data
      stride_in_bytes,                             // byte-offset between consecutive vertex attributes
      reinterpret_cast<GLvoid*>(offset_in_bytes)); // offset from beginning of buffer
  // clang-format on
  sc.components_skipped += component_count;
}
*/

template<typename L>
void
set_attrib_pointer(L &logger, optional_attribute const& optional_attribute_info, skip_context &sc)
{
  // * No point in setting an attribute if empty.
  //
  // (this has the effect of not incrementing the number of components skipped any in the "skip-
  // context")
  if (!optional_attribute_info) {
    return;
  }

  auto const &attribute_info = *optional_attribute_info;
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
}

} // ns impl

namespace global
{

template <typename L>
void
set_vertex_attributes(L &logger, vertex_attribute const &va)
{
  vao_bind(va.vao());
  ON_SCOPE_EXIT([]() { vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, va.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  impl::skip_context sc{va.num_floats()};
  impl::set_attrib_pointer(logger, va.vertices(), sc);
  impl::set_attrib_pointer(logger, va.colors(), sc);
  impl::set_attrib_pointer(logger, va.uv(), sc);
}

} // ns global

} // ns engine::gfx::opengl
