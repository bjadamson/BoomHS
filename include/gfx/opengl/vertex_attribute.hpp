#pragma once
#include <gfx/opengl/glsl.hpp>
#include <numeric>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace engine::gfx::opengl
{

struct attribute_info {
  GLint global_index;
  GLint num_components;
  GLenum type;
  char const *name;

  // MOVE_CONSTRUCTIBLE_ONLY(attribute_info);
  explicit constexpr attribute_info(GLint const i, GLint const nc, GLenum const ty, char const *n)
      : global_index(i)
      , num_components(nc)
      , type(ty)
      , name(n)
  {
  }

  MOVE_DEFAULT(attribute_info);
  COPY_DEFAULT(attribute_info);
  // NO_COPY(attribute_info);

  static constexpr auto A_POSITION = "a_position";
  static constexpr auto A_COLOR = "a_color";
  static constexpr auto A_UV = "a_uv";
};

template <std::size_t N>
class attribute_list
{
  std::array<attribute_info, N> list_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(attribute_list);
  attribute_list(std::array<attribute_info, N> &&attributes)
      : list_{std::move(attributes)}
  {
  }

  BEGIN_END_FORWARD_FNS(this->list_);
};

class vertex_attribute
{
  stlw::sized_buffer<attribute_info> list_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(vertex_attribute);

  template <std::size_t N>
  vertex_attribute(attribute_list<N> &&list)
      : list_(list.begin(), list.end())
  {
  }

  auto num_components() const
  {
    auto accumulator{0};
    auto const count_components = [](auto const &attrib_info) {
      return attrib_info.num_components;
    };
    for (auto const &it : this->list_) {
      accumulator += count_components(it);
    }
    return accumulator;
  }
  BEGIN_END_FORWARD_FNS(this->list_);
};

namespace impl
{

template <typename... Attributes>
auto
make_attribute_list(Attributes &&... attributes)
{
  static constexpr auto N = sizeof...(attributes);
  auto arr = stlw::make_array<attribute_info, N>(std::forward<Attributes>(attributes)...);
  return attribute_list<N>{std::move(arr)};
}

template <typename L, typename... Attributes>
auto
make_vertex_array(L &logger, Attributes &&... attributes)
{
  auto constexpr NUM_ATTRIBUTES = sizeof...(attributes);
  logger.trace(fmt::sprintf("Constructing vertex array from '%d' attributes.", NUM_ATTRIBUTES));

  {
    int max_attribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);

    auto const fmt = fmt::sprintf(
        "Queried OpengGL for maximum number of vertex attributes, found '%d'", max_attribs);
    logger.trace(fmt);

    if (max_attribs <= NUM_ATTRIBUTES) {
      auto const fmt =
          fmt::sprintf("Error requested '%d' vertex attributes from opengl, only '%d' available",
                       NUM_ATTRIBUTES, max_attribs);
      logger.error(fmt);
      assert(false);
    }
  }

  auto list = make_attribute_list(std::forward<Attributes>(attributes)...);
  return vertex_attribute{std::move(list)};
}

struct skip_context {
  GLsizei const total_component_count;
  GLsizei components_skipped = 0;

  explicit skip_context(GLsizei const c)
      : total_component_count(c)
  {
  }
};

template <typename L>
void
set_attrib_pointer(L &logger, attribute_info const &attrib_info, skip_context &sc)
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
  sc.components_skipped += component_count;

  auto const make_decimals = [](auto const a0, auto const a1, auto const a2, auto const a3, auto const a4,
      auto const a5) {
    return fmt::sprintf("%-15d %-15d %-15d %-15d %-15d %-15d", a0, a1, a2, a3, a4, a5);
  };

  auto const make_strings = [](auto const a0, auto const a1, auto const a2, auto const a3, auto const a4,
      auto const a5) {
    return fmt::sprintf("%-15s %-15s %-15s %-15s %-15s %-15s", a0, a1, a2, a3, a4, a5);
  };

  auto const s = make_decimals(attribute_index, component_count, DONT_NORMALIZE_THE_DATA,
      stride_in_bytes, offset_in_bytes, sc.components_skipped);
  auto const z = make_strings("attribute_index", "component_count", "normalize_data", "stride", "offset",
      "component_sk");

  logger.debug(z);
  logger.debug(s);
}

template<typename L>
auto
make_vertex_only_vertex_attribute(L &logger, GLint const num_fields_vertex)
{
  // attribute indexes
  constexpr auto V_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_POSITION;

  using ai = attribute_info;
  attribute_info vertex_info{V_INDEX,  num_fields_vertex, GL_FLOAT, ai::A_POSITION};

  return make_vertex_array(logger, std::move(vertex_info));
}

} // ns impl

namespace global
{

template<typename L>
auto
make_vertex_color_vertex_attribute(L &logger)
{
  // attribute indexes
  constexpr auto V_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_POSITION;
  constexpr auto C_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_COLOR;

  // num fields per attribute
  GLint constexpr num_fields_vertex = 4; // x, y, z, w
  GLint constexpr num_fields_color = 4;  // r, g, b, a

  using ai = attribute_info;
  attribute_info constexpr vertex_info{V_INDEX,  num_fields_vertex, GL_FLOAT, ai::A_POSITION};
  attribute_info constexpr color_info {C_INDEX,  num_fields_color,  GL_FLOAT, ai::A_COLOR};

  return impl::make_vertex_array(logger, vertex_info, color_info);
}

template<typename L>
auto
make_vertex_uv2d_vertex_attribute(L &logger)
{
  // attribute indexes
  constexpr auto V_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_POSITION;
  constexpr auto UV_INDEX = VERTEX_ATTRIBUTE_INDEX_OF_UV;

  // num fields per attribute
  GLint constexpr num_fields_vertex = 4; // x, y, z, w
  GLint constexpr num_fields_uv = 2;     // u, v

  using ai = attribute_info;
  attribute_info constexpr vertex_info{V_INDEX,  num_fields_vertex, GL_FLOAT, ai::A_POSITION};
  attribute_info constexpr uv_info    {UV_INDEX, num_fields_uv,     GL_FLOAT, ai::A_UV};

  return impl::make_vertex_array(logger, vertex_info, uv_info);
}

template<typename L>
auto
make_3dvertex_only_vertex_attribute(L &logger)
{
  // num fields per attribute
  GLint constexpr num_fields_vertex = 4; // x, y, z, w
  return impl::make_vertex_only_vertex_attribute(logger, num_fields_vertex);
}

template<typename L>
auto
make_2dvertex_only_vertex_attribute(L &logger)
{
  // num fields per attribute
  // TODO: maybe 3 fields????
  GLint constexpr num_fields_vertex = 4; // x, y, z, w
  return impl::make_vertex_only_vertex_attribute(logger, num_fields_vertex);
}

template <typename L>
void
set_vertex_attributes(L &logger, vertex_attribute const& va)
{
  auto const log_info = [&logger](auto const& it) {
    GLint enabled = 0;
    glGetVertexAttribiv(it.global_index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    auto const sb = [](auto const& enabled) { return (GL_FALSE == enabled) ? "false" : "true"; };
    auto const fmt = fmt::sprintf("Querying OpengGL, vertex attribute index '%d' enabled: '%s'",
        it.global_index, sb(enabled));
    logger.trace(fmt);
  };

  impl::skip_context sc{va.num_components()};
  for (auto const& it : va) {
    logger.trace(fmt::sprintf("setting attribute '%d'", it.global_index));
    impl::set_attrib_pointer(logger, it, sc);
    log_info(it);
  }
}

} // ns global

} // ns engine::gfx::opengl
