#pragma once
#include <engine/gfx/opengl/glew.hpp>
#include <stlw/format.hpp>
#include <stlw/type_ctors.hpp>

#include <boost/optional.hpp>
#include <string> // TODO: rm

namespace engine::gfx::opengl::global::log::impl
{

// also, make an abstraction over the source, not just vector<char>
static std::string
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

static boost::optional<std::string>
get_errors(GLuint const program_id)
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

template<typename L>
auto
log_any_gl_errors(L &logger, std::string const& msg, int const line)
{
  GLenum const err = glGetError();
  if (err != GL_NO_ERROR) {
    auto const msg = fmt::sprintf("GL error detected (line %d), code: '%d', string: '%s'",
        line, err, gluErrorString(err));
    logger.error(msg);
    assert(false);
  }
};

inline void
clear_gl_errors()
{
  while (glGetError() != GL_NO_ERROR) {}
}

} // ns engine::gfx::opengl::global::log::impl

#define LOG_GL_ERRORS(logger, msg)                                                 \
  do {                                                                             \
    ::engine::gfx::opengl::global::log::impl::log_any_gl_errors(logger, msg, __LINE__); \
  } while (0)
