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
using namespace std;

namespace
{

boost::expected<std::string, std::string>
load_shader_from_cstring_path(char const* path)
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

} // ns anonymous

namespace engine
{
namespace gfx
{

boost::expected<GLuint, std::string>
compile_shader(char const* data, GLenum const type)
{
  GLuint const handle = glCreateShader(type);

  // TODO: move this into std::utils, something like std::vec::with_size() (also with_capacity())
  auto vec_with_size = [](auto const size) {
    std::vector<char> buffer;
    buffer.resize(size);
    return buffer;
  };

  // Compile Vertex Shader
  glShaderSource(handle, 1, &data, nullptr);
  glCompileShader(handle);

  // Check Vertex Shader
  GLint is_compiled = GL_FALSE;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &is_compiled);
  if (is_compiled) {
    return handle;
  }
  GLint InfoLogLength = 0;
  glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (0 > InfoLogLength) {
    auto constexpr err = "Compiling shader failed and could not retrieve OpenGL Log info for object";
    return boost::make_unexpected(err);
  }

  auto const buffer_size = InfoLogLength + 1; // +1 for null terminator character '\0'
  auto buffer = vec_with_size(buffer_size);
  glGetShaderInfoLog(handle, buffer_size, nullptr, buffer.data());
  return boost::make_unexpected( std::string{buffer.cbegin(), buffer.cend()} );
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
  DO_MONAD(auto vertex_shader_source, load_shader_from_cstring_path(vertex_file_path));

  // Read the Fragment Shader code from the file
  DO_MONAD(auto fragment_shader_source, load_shader_from_cstring_path(fragment_file_path));

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
