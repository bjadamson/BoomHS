#pragma once
#include <string> // TODO: rm

#include <engine/gfx/opengl_glew.hpp>
#include <stlw/format.hpp>
#include <stlw/type_ctors.hpp>
#include <boost/optional.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

class gl_log
{
  gl_log() = delete;

  // also, make an abstraction over the source, not just vector<char>
  inline static std::string
  retrieve(GLuint const handle, void (*f)(GLuint, GLsizei, GLsizei *, GLchar *))
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
      return "Error retrieving OpenGL Log info for this shader object.";
    }

    // Step 2
    auto const buffer_size = log_length + 1; // +1 for null terminator character '\0'
    auto buffer = stlw::vec_with_size<char>(buffer_size);
    f(handle, buffer_size, nullptr, buffer.data());

    // Step 3
    return std::string{buffer.cbegin(), buffer.cend()};
  }
public:
  inline static std::string
  get_shader_log(GLuint const handle) { return retrieve(handle, glGetShaderInfoLog); }

  inline static std::string
  get_program_log(GLuint const handle) { return retrieve(handle, glGetProgramInfoLog); }

  inline static boost::optional<std::string>
  get_opengl_errors(GLuint const program_id)
  {
    char buffer[2096];
    int actual_length = 0;
    glGetProgramInfoLog(program_id, 2096, &actual_length, buffer);
    if (actual_length <= 0) {
      return boost::none;
    }
    auto fmt = fmt::sprintf("Opengl error: '{}'", std::to_string(buffer[0]));
    return boost::make_optional(fmt);
  }
};

} // ns opengl
} // ns gfx
} // ns engine
