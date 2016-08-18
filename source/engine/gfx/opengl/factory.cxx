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

  // enable vertex attibute arrays
  global_enable_vattrib_array(RED_TRIANGLE_VERTEX_POSITION_INDEX);
  global_enable_vattrib_array(RED_TRIANGLE_VERTEX_COLOR_INDEX);
  global_enable_vattrib_array(RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX);

  glBindBuffer(GL_ARRAY_BUFFER, triangle.vbo_);
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  static auto constexpr VERTICE_COMPONENT_COUNT = 4;  // x, y, z, w
  static auto constexpr COLOR_COMPONENT_COUNT = 4;    // r, g, b, a
  static auto constexpr TEXCOORD_COMPONENT_COUNT = 2; // u, v
  static auto constexpr DONT_NORMALIZE_THE_DATA = GL_FALSE;
  static auto constexpr TOTAL_COMPONENT_COUNT = VERTICE_COMPONENT_COUNT + COLOR_COMPONENT_COUNT
    + TEXCOORD_COMPONENT_COUNT;

  auto const set_attrib_pointer = [](auto const attribute_index, auto const component_count,
      auto const offset, auto const num_component_skip)
  {
    // clang-format off
    auto const offset_in_bytes = (offset + num_component_skip) * sizeof(GL_FLOAT);
    glVertexAttribPointer(
        attribute_index,                             // global index id
        component_count,                             // number of components per attribute
        GL_FLOAT,                                    // data-type of the components
        DONT_NORMALIZE_THE_DATA,                     // don't normalize our data
        TOTAL_COMPONENT_COUNT * sizeof(GL_FLOAT),    // byte-offset between consecutive vertex attributes
        reinterpret_cast<GLvoid*>(offset_in_bytes)); // offset from beginning of buffer
    // clang-format on
    return component_count + num_component_skip;
  };

  // configure this OpenGL VAO attribute array
  int num_components = 0;
  {
    static auto constexpr VERTICE_ELEMENT_OFFSET = 0;
    static auto constexpr PREV_NUM_COMPONENTS = 0;
    num_components = set_attrib_pointer(RED_TRIANGLE_VERTEX_POSITION_INDEX, VERTICE_COMPONENT_COUNT,
        VERTICE_ELEMENT_OFFSET, PREV_NUM_COMPONENTS);
  }
  {
    static auto constexpr COLOR_ELEMENT_OFFSET = VERTICE_COMPONENT_COUNT;
    num_components = set_attrib_pointer(RED_TRIANGLE_VERTEX_COLOR_INDEX, COLOR_COMPONENT_COUNT,
        COLOR_ELEMENT_OFFSET, num_components);
  }
  {
    static auto constexpr TEXCOORD_ELEMENT_OFFSET = VERTICE_COMPONENT_COUNT + COLOR_COMPONENT_COUNT;
    num_components = set_attrib_pointer(RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX,
        TEXCOORD_COMPONENT_COUNT, TEXCOORD_ELEMENT_OFFSET, num_components);
  }

  // TODO: figure out why the compiler gets confused without the std::move() (why does it try to
  // copy instead of move the value?? Maybe c++17 (rvo guarantees) fixes this??)
  return std::move(triangle);
}

} // opengl
} // ns gfx
} // ns engine
