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

  global_enable_vattrib_array(RED_TRIANGLE_VERTEX_POSITION_INDEX);

  static auto constexpr NUM_VERTICES = 4;
  static auto constexpr TYPE_OF_DATA = GL_FLOAT;
  static auto constexpr NORMALIZE_DATA = GL_FALSE;
  static auto constexpr STRIDE_SIZE = NUM_VERTICES * sizeof(TYPE_OF_DATA);
  static auto constexpr OFFSET_PTR = nullptr;

  glBindBuffer(GL_ARRAY_BUFFER, triangle.vbo_);
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  // configure this OpenGL VAO attribute array
  glVertexAttribPointer(RED_TRIANGLE_VERTEX_POSITION_INDEX, NUM_VERTICES, TYPE_OF_DATA,
                        NORMALIZE_DATA, STRIDE_SIZE, OFFSET_PTR);

  // TODO: figure out why the compiler gets confused without the std::move() (why does it try to
  // copy instead of move the value?? Maybe c++17 (rvo guarantees) fixes this??)
  return std::move(triangle);
}

} // opengl
} // ns gfx
} // ns engine
