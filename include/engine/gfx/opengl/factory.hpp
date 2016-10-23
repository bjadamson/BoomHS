#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/polygon_renderer.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

namespace impl {

struct vertex_shader_filename {
  char const *filename;
  vertex_shader_filename(char const *f)
      : filename(f)
  {
  }
};
struct fragment_shader_filename {
  char const *filename;
  fragment_shader_filename(char const *f)
      : filename(f)
  {
  }
};

namespace gl = engine::gfx::opengl;

stlw::result<gl::program, std::string>
load_program(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto expected_program_id = gl::program_loader::load(v.filename, f.filename);
  if (!expected_program_id) {
    return stlw::make_error(expected_program_id.error());
  }
  return expected_program_id;
}

auto const global_enable_vattrib_array = [](auto const index) { glEnableVertexAttribArray(index); };
} // ns impl

class factory
{
  NO_COPY_AND_NO_MOVE(factory);

  static auto constexpr RED_TRIANGLE_VERTEX_POSITION_INDEX = 0;
  static auto constexpr RED_TRIANGLE_VERTEX_COLOR_INDEX = 1;
  static auto constexpr RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX = 2;

public:
  template<typename L>
  static stlw::result<polygon_renderer, std::string>
  make_polygon_renderer(L &logger)
  {
    DO_MONAD(auto phandle, impl::load_program("shader.vert", "shader.frag"));
    polygon_renderer polygon_renderer{std::move(phandle)};

    global_vao_bind(polygon_renderer.vao_);
    ON_SCOPE_EXIT([]() { global_vao_unbind(); });

    glBindBuffer(GL_ARRAY_BUFFER, polygon_renderer.vbo_);
    ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

    struct skip_context {
      GLsizei const total_component_count;
      GLsizei components_skipped = 0;

      skip_context(GLsizei const c)
          : total_component_count(c)
      {
      }
    };

    auto const set_attrib_pointer = [&logger](auto const attribute_index, auto const component_count,
                                      skip_context &sc) {
      // enable vertex attibute arrays
      impl::global_enable_vattrib_array(attribute_index);

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

    // configure this OpenGL VAO attribute array
    static auto constexpr VERTICE_COMPONENT_COUNT = 4;  // x, y, z, w
    static auto constexpr COLOR_COMPONENT_COUNT = 4;    // r, g, b, a
    static auto constexpr TEXCOORD_COMPONENT_COUNT = 2; // u, v
    static auto constexpr COMPONENT_COUNT =
        VERTICE_COMPONENT_COUNT + COLOR_COMPONENT_COUNT + TEXCOORD_COMPONENT_COUNT;

    skip_context sc{COMPONENT_COUNT};
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
    set_attrib_pointer(RED_TRIANGLE_VERTEX_POSITION_INDEX, VERTICE_COMPONENT_COUNT, *&sc);
    set_attrib_pointer(RED_TRIANGLE_VERTEX_COLOR_INDEX, COLOR_COMPONENT_COUNT, *&sc);
    set_attrib_pointer(RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX, TEXCOORD_COMPONENT_COUNT, *&sc);
    return polygon_renderer;
  }
};

} // opengl
} // ns gfx
} // ns engine
