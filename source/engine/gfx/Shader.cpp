#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <engine/gfx/Shader.hpp>

namespace
{

// TODO: move to util function
boost::expected<std::string, std::string>
read_file(char const* path)
{
  // Read the Vertex Shader code from the file
  std::ifstream istream(path, std::ios::in);
  std::stringstream sstream;

  if (! istream.is_open()) {
    return boost::make_unexpected("Error opening file at path '" + std::string{path} + "'");
  }

  std::string next_line = "";
  while(std::getline(istream, next_line)) {
    sstream << "\n" << next_line;
  }
  // explicit, dtor should do this.
  istream.close();
  return sstream.str();
}

inline bool
is_compiled(GLuint const handle)
{
  GLint is_compiled = GL_FALSE;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &is_compiled);
  return GL_FALSE != is_compiled;
}

inline void
gl_compile_shader(GLuint const handle, char const* source)
{
  GLint *p_length = nullptr;
  auto constexpr shader_count = 1; // We're only compiling one shader (in this function anyway).
  glShaderSource(handle, 1, &source, p_length);
  glCompileShader(handle);
}

} // ns anonymous

namespace engine
{
namespace gfx
{

// TODO: export as reusable fn
// also, make an abstraction over the source, not just vector<char>
inline std::string
retrieve_shader_log(GLuint const handle)
{
  // We have to do a low-level dance to get the OpenGL shader logs.
  //
  // 1. Ask OpenGL how buffer space we need to retrieve the buffer.
  // 2. Retrieve the data into our buffer.
  // 3. Return the buffer.

  // TODO: move this into std::utils, something like std::vec::with_size() (also with_capacity())
  auto vec_with_size = [](auto const size) {
    std::vector<char> buffer;
    buffer.resize(size);
    return buffer;
  };

  // Step 1
  GLint log_length = 0;
  glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
  if (0 > log_length) {
    // fix this msg
    return "Compiling shader failed and could not retrieve OpenGL Log info for object";
  }

  // Step 2
  auto const buffer_size = log_length + 1; // +1 for null terminator character '\0'
  auto buffer = vec_with_size(buffer_size);
  glGetShaderInfoLog(handle, buffer_size, nullptr, buffer.data());

  // Step 3
  return std::string{buffer.cbegin(), buffer.cend()};
}

boost::expected<GLuint, std::string>
compile_shader(char const* data, GLenum const type)
{
  GLuint const handle = glCreateShader(type);
  gl_compile_shader(handle, data);

  // Check Vertex Shader
  if (true == is_compiled(handle)) {
    return handle;
  }
  return boost::make_unexpected(retrieve_shader_log(handle));
}

template<typename T>
boost::expected<GLuint, std::string>
compile_shader(T const& data, GLenum const type)
{ return compile_shader(data.c_str(), type); }

boost::expected<GLuint, std::string>
LoadShaders(char const* vertex_file_path, char const* fragment_file_path)
{
  // Create the shaders
#define ONLY_OK(VAR_DECL, V, expr) \
    auto V = expr; \
    if (! V) { return boost::make_unexpected(V.error()); } \
    VAR_DECL = *V;

#define ONLY_OK_HELPER(VAR_DECL, V, expr) \
  ONLY_OK(VAR_DECL, expected_##V, expr)

#define ONLY_IFOK_HELPER(VAR_DECL, to_concat, expr) \
  ONLY_OK_HELPER(VAR_DECL, to_concat, expr)

#define DO_MONAD(VAR_DECL, expr) ONLY_IFOK_HELPER(VAR_DECL, __COUNTER__, expr)

  // Read the Vertex Shader code from the file
  DO_MONAD(auto vertex_shader_source, read_file(vertex_file_path));

  // Read the Fragment Shader code from the file
  DO_MONAD(auto fragment_shader_source, read_file(fragment_file_path));

  DO_MONAD(GLuint const VertexShaderID, compile_shader(vertex_shader_source, GL_VERTEX_SHADER));
  DO_MONAD(GLuint const FragmentShaderID, compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER));

  // Link the program
  printf("Linking program\n");
  GLuint const ProgramID = glCreateProgram();
  if (0 == ProgramID) {
    return boost::make_unexpected("GlCreateProgram returned 0.");
  }
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);

  // Check the program
  GLint Result;
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  if (Result == GL_FALSE) {
    printf("%s\n", "Linking the shader failed.");
  }
  GLint InfoLogLength;
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if ( InfoLogLength > 1 ) {
    std::vector<char> ProgramErrorMessage(InfoLogLength+1);
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    printf("there was an error.");
    printf("%s\n", &ProgramErrorMessage[0]);
  }

  glDetachShader(ProgramID, VertexShaderID);
  glDetachShader(ProgramID, FragmentShaderID);

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);
  return ProgramID;
}

} // ns gfx
} // ns engine
