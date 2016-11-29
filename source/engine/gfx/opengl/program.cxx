#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/glsl.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/vertex_attribute.hpp>
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
  return stlw::make_error(global::log::get_shader_log(handle));
}

template <typename T>
stlw::result<compiled_shader, std::string>
compile_shader(T const &data, GLenum const type)
{
  return compile_shader(data.c_str(), type);
}

inline stlw::result<GLuint, std::string>
create_program()
{
  GLuint const program_id = glCreateProgram();
  if (0 == program_id) {
    return stlw::make_error("GlCreateProgram returned 0.");
  }
  return program_id;
}

inline stlw::result<stlw::empty_type, std::string>
link_program(GLuint const program_id)
{
  // Link the program
  glLinkProgram(program_id);

  // Check the program
  GLint result;
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    return stlw::make_error("Linking the shader failed. Progam log '" +
                            global::log::get_program_log(program_id) + "'");
  }
  return stlw::make_empty();
}

stlw::result<program, std::string>
compile_sources(std::string const &vertex_shader_source, std::string const &fragment_shader_source)
{
  DO_TRY(auto const vertex_shader_id, compile_shader(vertex_shader_source, GL_VERTEX_SHADER));
  DO_TRY(auto const frag_shader_id, compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER));
  DO_TRY(auto const program_id, create_program());

  glBindAttribLocation(program_id, VERTEX_ATTRIBUTE_INDEX_OF_POSITION, attribute_info::A_POSITION);
  glBindAttribLocation(program_id, VERTEX_ATTRIBUTE_INDEX_OF_COLOR, attribute_info::A_COLOR);
  glBindAttribLocation(program_id, VERTEX_ATTRIBUTE_INDEX_OF_UV, attribute_info::A_UV);

  glAttachShader(program_id, vertex_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, vertex_shader_id); });

  glAttachShader(program_id, frag_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, frag_shader_id); });

  DO_EFFECT(link_program(program_id));
  return program::make(program_id);
}

} // ns anonymous

namespace engine::gfx::opengl
{

void
program::use()
{
  auto const &p = this->program_id_;

  /*
  GLint num_active_attributes;
  glGetProgramiv(p, GL_ACTIVE_ATTRIBUTES, &num_active_attributes);

  auto constexpr BUFFER_LENGTH = GL_ACTIVE_ATTRIBUTE_MAX_LENGTH;

  GLsizei *length = nullptr; // This is OK if we don't care about the length according to docs.
  GLint size;
  GLenum type;
  GLchar name[BUFFER_LENGTH] = {0};
  for (auto i{0}; i < BUFFER_LENGTH; ++i) {
    glGetActiveAttrib(p, i, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, length, &size, &type, &name[0]);
  }
  */

  glUseProgram(p);
}

stlw::result<program, std::string>
program_loader::from_files(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto const prefix = [](auto const &path) {
    return std::string{"./build-system/bin/shaders/"} + path;
  };
  auto const vertex_shader_path = prefix(v.filename);
  auto const fragment_shader_path = prefix(f.filename);

  // Read the Vertex/Fragment Shader code from ther file
  DO_TRY(auto const vertex_shader_source, stlw::read_file(vertex_shader_path));
  DO_TRY(auto const fragment_shader_source, stlw::read_file(fragment_shader_path));

  return compile_sources(vertex_shader_source, fragment_shader_source);
}

} // ns engine::gfx::opengl
