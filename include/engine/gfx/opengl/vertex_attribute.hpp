#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/glsl.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <numeric>

namespace engine::gfx::opengl
{

struct attribute_info {
  GLint global_index;
  GLint num_components;
  GLenum type;
  char const* name;

  explicit constexpr attribute_info(GLint const i, GLint const nc, GLenum const ty, char const* n)
      : global_index(i)
      , num_components(nc)
      , type(ty)
      , name(n)
  {
  }

  MOVE_DEFAULT(attribute_info);
  COPY_DEFAULT(attribute_info);

  static constexpr auto A_POSITION = "a_position";
  static constexpr auto A_COLOR = "a_color";
  static constexpr auto A_UV = "a_uv";
};

template<std::size_t N>
class attribute_list {
  std::array<attribute_info, N> const list_;

public:
  constexpr attribute_list() : list_(std::array<attribute_info,  N>{}) {}
  explicit constexpr attribute_list(std::array<attribute_info, N> const& attributes)
    : list_{attributes}
  {}

  MOVE_DEFAULT(attribute_list);
  COPY_DEFAULT(attribute_list);

  auto num_components() const
  {
    auto accumulator{0};
    auto const count_components =
      [&accumulator](auto const& attrib_info) { return attrib_info.num_components; };
    for (auto const& it: this->list_) {
      accumulator += count_components(it);
    }
    return accumulator;
  }

  decltype(auto) begin() const { return this->list_.begin(); }
  decltype(auto) end() const { return this->list_.end(); }
};

template<std::size_t N>
class vertex_attribute
{
  GLuint const vao_, vbo_;
  attribute_list<N> const list_;

public:
  vertex_attribute(GLuint const vao, GLuint const vbo, attribute_list<N> const &list)
      : vao_(vao)
      , vbo_(vbo)
      , list_(list)
  {
  }

  auto const vao() const { return this->vao_; }
  auto const vbo() const { return this->vbo_; }
  auto num_components() const { return this->list_.num_components(); }

  decltype(auto) begin() const { return this->list_.begin(); }
  decltype(auto) end() const { return this->list_.end(); }
};

namespace impl
{

template<typename ...Args>
auto
make_attribute_list(Args &&... args)
{
  static constexpr auto N = sizeof...(args);
  auto arr = stlw::make_array<attribute_info, N>(args...);
  return attribute_list<N>{arr};
}

template<typename ...Args>
auto
make_vertex_array(GLuint const vao, GLuint const vbo, Args &&... args)
{
  static constexpr auto N = sizeof...(args);
  auto list = make_attribute_list(std::forward<Args>(args)...);
  return vertex_attribute<N>{vao, vbo, std::move(list)};
}

struct skip_context {
  GLsizei const total_component_count;
  GLsizei components_skipped = 0;

  explicit skip_context(GLsizei const c)
      : total_component_count(c)
  {
  }
};

template<typename L>
void
set_attrib_pointer(L &logger, attribute_info const& attrib_info, skip_context &sc)
{
  // enable vertex attibute arrays
  auto const attribute_index = attrib_info.global_index;
  auto const attribute_type = attrib_info.type;
  glEnableVertexAttribArray(attribute_index);

  static auto constexpr DONT_NORMALIZE_THE_DATA = GL_FALSE;

  // clang-format off
  auto const offset_in_bytes = sc.components_skipped * sizeof(GL_FLOAT);
  auto const stride_in_bytes = sc.total_component_count * sizeof(GL_FLOAT);
  auto const component_count = attrib_info.num_components;

  glVertexAttribPointer(
      attribute_index,                             // global index id
      component_count,                             // number of components per attribute
      attrib_info.type,                            // data-type of the components
      DONT_NORMALIZE_THE_DATA,                     // don't normalize our data
      stride_in_bytes,                             // byte-offset between consecutive vertex attributes
      reinterpret_cast<GLvoid*>(offset_in_bytes)); // offset from beginning of buffer
  // clang-format on
  sc.components_skipped += component_count;

  GLint enabled = 0;
  glGetVertexAttribiv(attribute_index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);

  auto const s = fmt::format("%-10d %-10d %-10d %-10s %-10d %-10d %-10d %-10d\n", attribute_index,
                              enabled, component_count, "TODO()", DONT_NORMALIZE_THE_DATA,
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

inline auto
make_color_uv_vertex_attribute(opengl_context const &ctx)
{
  // attribute indexes
  constexpr auto V_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_POSITION;
  constexpr auto C_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_COLOR;
  constexpr auto UV_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_UV;

  // clang-format off
  // num fields per attribute
  GLint constexpr num_fields_vertex = 4; // x, y, z, w
  GLint constexpr num_fields_color = 4;  // r, g, b, a
  GLint constexpr num_fields_uv = 2;     // u, v

  using ai = attribute_info;
  attribute_info constexpr vertex_info{V_INDEX,  num_fields_vertex, GL_FLOAT, ai::A_POSITION};
  attribute_info constexpr color_info {C_INDEX,  num_fields_color,  GL_FLOAT, ai::A_COLOR};
  attribute_info constexpr uv_info    {UV_INDEX, num_fields_uv,     GL_FLOAT, ai::A_UV};
  // clang-format on

  return impl::make_vertex_array(ctx.vao(), ctx.vbo(), vertex_info, color_info, uv_info);
}

template <typename L, std::size_t N>
void
set_vertex_attributes(L &logger, vertex_attribute<N> const &va)
{
  vao_bind(va.vao());
  ON_SCOPE_EXIT([]() { vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, va.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  impl::skip_context sc{va.num_components()};
  for (auto const& it : va) {
    impl::set_attrib_pointer(logger, it, sc);
  }
}

} // ns global

} // ns engine::gfx::opengl
