#include <engine/gfx/opengl/gl.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <stlw/os.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace
{
using compiled_shader = stlw::ImplicitelyCastableMovableWrapper<GLuint, decltype(glDeleteShader)>;
using namespace engine::gfx::opengl;

inline bool
is_compiled(GLuint const handle)
{
  GLint is_compiled = GL_FALSE;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &is_compiled);
  return GL_FALSE != is_compiled;
}

inline void
gl_compile_shader(GLuint const handle, char const *source)
{
  GLint *p_length = nullptr;
  auto constexpr SHADER_COUNT = 1; // We're only compiling one shader (in this function anyway).
  glShaderSource(handle, SHADER_COUNT, &source, p_length);
  glCompileShader(handle);
}

stlw::result<compiled_shader, std::string>
compile_shader(char const *data, GLenum const type)
{
  GLuint const handle = glCreateShader(type);
  gl_compile_shader(handle, data);

  // Check Vertex Shader
  if (true == is_compiled(handle)) {
    return compiled_shader{handle, glDeleteShader};
  }
  return stlw::make_error(gl_log::get_shader_log(handle));
}

template <typename T>
stlw::result<compiled_shader, std::string>
compile_shader(T const &data, GLenum const type)
{
  return compile_shader(data.c_str(), type);
}

stlw::result<GLuint, std::string>
create_program()
{
  GLuint const program_id = glCreateProgram();
  if (0 == program_id) {
    return stlw::make_error("GlCreateProgram returned 0.");
  }
  return program_id;
}

stlw::result<stlw::empty_type, std::string>
link_program(GLuint const program_id)
{
  // Link the program
  glLinkProgram(program_id);

  // Check the program
  GLint result;
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    return stlw::make_error("Linking the shader failed. Progam log '" +
                            gl_log::get_program_log(program_id) + "'");
  }
  return stlw::make_empty();
}

} // ns anonymous

namespace engine
{
namespace gfx
{
namespace opengl
{

stlw::result<program, std::string>
program_loader::load(char const *vertex_file_path, char const *fragment_file_path)
{
  // Read the Vertex/Fragment Shader code from ther file
  DO_MONAD(auto const vertex_shader_source, stlw::read_file(vertex_file_path));
  DO_MONAD(auto const fragment_shader_source, stlw::read_file(fragment_file_path));

  DO_MONAD(auto const vertex_shader_id, compile_shader(vertex_shader_source, GL_VERTEX_SHADER));
  DO_MONAD(auto const frag_shader_id, compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER));
  DO_MONAD(auto const program_id, create_program());

  glAttachShader(program_id, vertex_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, vertex_shader_id); });

  glAttachShader(program_id, frag_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, frag_shader_id); });

  DO_EFFECT(link_program(program_id));
  return program::make(program_id);
}

} // ns opengl
} // ns gfx
} // ns engine
