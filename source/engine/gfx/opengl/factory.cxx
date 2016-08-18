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

  static auto constexpr DONT_NORMALIZE_THE_DATA = GL_FALSE;
  static auto constexpr VERTEX_ELEMENT_COUNT             = 4; // x, y, z, w
  static auto constexpr COLOR_ELEMENT_COUNT              = 4; // r, g, b, a
  static auto constexpr TEXTURE_COORDINATE_ELEMENT_COUNT = 2; // u, v
  static auto constexpr STRIDE_DISTANCE =
      (VERTEX_ELEMENT_COUNT + COLOR_ELEMENT_COUNT + TEXTURE_COORDINATE_ELEMENT_COUNT) * sizeof(GL_FLOAT);

  // list offsets sequentially
  static auto constexpr VERTICE_OFFSET = 0;

  static auto constexpr COLOR_OFFSET =
    VERTICE_OFFSET + (VERTEX_ELEMENT_COUNT * sizeof(GL_FLOAT));

  static auto constexpr TEXTURE_COORDINATE_OFFSET =
    COLOR_OFFSET + (COLOR_ELEMENT_COUNT * sizeof(GL_FLOAT));

  // configure this OpenGL VAO attribute array
  // clang-format off
  glVertexAttribPointer(
      RED_TRIANGLE_VERTEX_POSITION_INDEX,           // attribute
      VERTEX_ELEMENT_COUNT,                         // number of floats per-element
      GL_FLOAT,                                     // type of each vertice-element
      DONT_NORMALIZE_THE_DATA,                      // normalize our data
      STRIDE_DISTANCE,                              // next "'vertice'" is every n floats
      reinterpret_cast<GLvoid*>(VERTICE_OFFSET));   // offset from beginning of buffer

  glVertexAttribPointer(
      RED_TRIANGLE_VERTEX_COLOR_INDEX,              // attribute
      COLOR_ELEMENT_COUNT,                          // number of elements per-color
      GL_FLOAT,                                     // type of each color-element
      DONT_NORMALIZE_THE_DATA,                      // normalize our data
      STRIDE_DISTANCE,                              // next "'color'" is every n floats
      reinterpret_cast<GLvoid*>(COLOR_OFFSET));     // offset from beginning of buffer

  glVertexAttribPointer(
      RED_TRIANGLE_VERTEX_TEXTURE_COORDINATE_INDEX, // attribute
      TEXTURE_COORDINATE_ELEMENT_COUNT,             // number of floats per-element
      GL_FLOAT,                                     // type of each color-element
      DONT_NORMALIZE_THE_DATA,                      // normalize our data
      STRIDE_DISTANCE,                              // next "'texture coordinate'" is every n floats
      reinterpret_cast<GLvoid*>(TEXTURE_COORDINATE_OFFSET)); // offset from beginning of buffer
  // clang-format on

  // TODO: figure out why the compiler gets confused without the std::move() (why does it try to
  // copy instead of move the value?? Maybe c++17 (rvo guarantees) fixes this??)
  return std::move(triangle);
}

} // opengl
} // ns gfx
} // ns engine
