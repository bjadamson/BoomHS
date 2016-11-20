#pragma once
#include <engine/gfx/opengl/gl.hpp>
#include <engine/gfx/opengl/vertex_attrib.hpp>

// Functions within this namespace have global side effects within OpenGL's internal state.
//
// Here be dragons.
namespace engine::gfx::opengl::global
{
  static auto const vao_bind = [](auto const vao) { glBindVertexArray(vao); };
  static auto const vao_unbind = []() { glBindVertexArray(0); };

  static auto const texture_bind = [](auto const tid) { glBindTexture(GL_TEXTURE_2D, tid); };
  static auto const texture_unbind = []() { glBindTexture(GL_TEXTURE_2D, 0); };

  static auto const set_vertex_attributes = [](auto &logger, vertex_attribute_config const& config)
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

} // ns engine::gfx::opengl::global
