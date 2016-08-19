#include <engine/gfx/opengl/factory.hpp>
#include <stlw/type_macros.hpp>

namespace gl = engine::gfx::opengl;

namespace
{

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

stlw::result<gl::program_handle, std::string>
load_program(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto expected_program_id = gl::program_loader::load(v.filename, f.filename);
  if (!expected_program_id) {
    return stlw::make_error(expected_program_id.error());
  }
  return expected_program_id;
}

auto const global_enable_vattrib_array = [](auto const index) { glEnableVertexAttribArray(index); };

} // ns anon

namespace engine
{
namespace gfx
{
namespace opengl
{

stlw::result<red_triangle, std::string>
factory::make_red_triangle_program()
{
  DO_MONAD(auto phandle, load_program("shader.vert", "shader.frag"));
  red_triangle triangle{std::move(phandle)};

  global_vao_bind(triangle.vao_);
  ON_SCOPE_EXIT([]() { global_vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, triangle.vbo_);
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  struct skip_context {
    GLsizei const total_component_count;
    GLsizei components_skipped = 0;

    skip_context(GLsizei const c)
        : total_component_count(c)
    {
    }
  };

  auto const set_attrib_pointer = [](auto const attribute_index, auto const component_count,
                                     skip_context &sc) {
    // enable vertex attibute arrays
    global_enable_vattrib_array(attribute_index);

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
        fmt::sprintf("%-10d %-10d %-10d %-10s %-10d %-10d %-10d %-10d\n", attribute_index, enabled,
                     component_count, "float", DONT_NORMALIZE_THE_DATA, stride_in_bytes,
                     offset_in_bytes, sc.components_skipped);
    std::cerr << s;
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
    auto const f = fmt::sprintf("maximum number of vertex attributes: '%d'\n", max_attribs);
    std::cerr << f;
  }
  {
    auto const f = fmt::sprintf("%-10s %-10s %-10s %-10s %-10s %-10s %-10s\n", "index:", "enabled",
                                "size", "type", "normalized", "stride", "pointer", "num skipped");
    std::cerr << f;
  }
  set_attrib_pointer(RED_TRIANGLE_VERTEX_POSITION_INDEX, VERTICE_COMPONENT_COUNT, *&sc);
  set_attrib_pointer(RED_TRIANGLE_VERTEX_COLOR_INDEX, COLOR_COMPONENT_COUNT, *&sc);
  set_attrib_pointer(RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX, TEXCOORD_COMPONENT_COUNT, *&sc);

  // TODO: figure out why the compiler gets confused without the std::move() (why does it try to
  // copy instead of move the value?? Maybe c++17 (rvo guarantees) fixes this??)
  return std::move(triangle);
}

} // opengl
} // ns gfx
} // ns engine
