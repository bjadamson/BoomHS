#include <engine/gfx/opengl/program.hpp>
#include <stlw/os.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace
{
using compiled_shader = stlw::ImplicitelyCastableMovableWrapper<GLuint, decltype(glDeleteShader)>;

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
  auto constexpr shader_count = 1; // We're only compiling one shader (in this function anyway).
  glShaderSource(handle, 1, &source, p_length);
  glCompileShader(handle);
}

// TODO: export as reusable fn
// also, make an abstraction over the source, not just vector<char>
inline std::string
retrieve_gl_log(GLuint const handle, void (*f)(GLuint, GLsizei, GLsizei *, GLchar *))
{
  // We have to do a low-level dance to get the OpenGL shader logs.
  //
  // 1. Ask OpenGL how buffer space we need to retrieve the buffer.
  // 2. Retrieve the data into our buffer.
  // 3. Return the buffer.
  // Step 1
  GLint log_length = 0;
  glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
  if (0 > log_length) {
    // fix this msg
    return "Compiling shader failed and could not retrieve OpenGL Log info for "
           "object";
  }

  // Step 2
  auto const buffer_size = log_length + 1; // +1 for null terminator character '\0'
  auto buffer = stlw::vec_with_size<char>(buffer_size);
  f(handle, buffer_size, nullptr, buffer.data());

  // Step 3
  return std::string{buffer.cbegin(), buffer.cend()};
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
  auto const get_shader_log = [](auto const handle) {
    return retrieve_gl_log(handle, glGetShaderInfoLog);
  };
  return stlw::make_error(get_shader_log(handle));
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
  printf("Linking program\n");
  glLinkProgram(program_id);
  auto const dump_program_log = [](auto const program_id, char const *prefix) {
    auto const get_program_log = [](auto const id) {
      return retrieve_gl_log(id, glGetProgramInfoLog);
    };
    auto const program_log = get_program_log(program_id);
    printf("'%s': %s\n", prefix, program_log.data());
  };

  // Check the program
  GLint Result;
  glGetProgramiv(program_id, GL_LINK_STATUS, &Result);
  if (Result == GL_FALSE) {
    return stlw::make_error("Linking the shader failed.");
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

stlw::result<program_handle, std::string>
program_loader::load(char const *vertex_file_path, char const *fragment_file_path)
{
  // Read the Vertex/Fragment Shader code from ther file
  DO_MONAD(auto vertex_shader_source, stlw::read_file(vertex_file_path));
  DO_MONAD(auto fragment_shader_source, stlw::read_file(fragment_file_path));

  DO_MONAD(auto const vertex_shader_id, compile_shader(vertex_shader_source, GL_VERTEX_SHADER));
  DO_MONAD(auto const frag_shader_id, compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER));
  DO_MONAD(auto const program_id, create_program());

  glAttachShader(program_id, vertex_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, vertex_shader_id); });

  glAttachShader(program_id, frag_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, frag_shader_id); });

  DO_EFFECT(link_program(program_id));
  return program_handle::make(program_id);
}

} // ns opengl
} // ns gfx
} // ns engine
