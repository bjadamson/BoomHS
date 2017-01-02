#pragma once
#include <gfx/opengl/gl_log.hpp>
#include <gfx/opengl/glew.hpp>
#include <gfx/opengl/global.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stlw/format.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#define DEFINE_SHADER_FILENAME_TYPE(NAME)                                                          \
  struct NAME##_shader_filename {                                                                  \
    char const *filename;                                                                          \
    NAME##_shader_filename(char const *f)                                                          \
        : filename(f)                                                                              \
    {                                                                                              \
    }                                                                                              \
  }

#include <iostream>
namespace gfx::opengl
{

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);

#undef DEFINE_SHADER_FILENAME_TYPE

constexpr GLuint
INVALID_PROGRAM_ID() { return 0; }

constexpr bool
is_invalid(GLuint const p) { return p == INVALID_PROGRAM_ID(); }

template <typename L>
void
check_opengl_errors(L &logger, GLuint const p)
{
  auto const errors = global::log::get_errors(p);
  if (errors) {
    logger.error("Opengl error: '{}'", *errors);
  }
}

inline void
destroy_program(GLuint const p)
{
  glDeleteProgram(p);
}

inline void
use_program(GLuint const p)
{
  glUseProgram(p);
}

namespace impl
{

template <typename L>
auto
get_uniform_location(L &logger, GLuint const p, GLchar const *name)
{
  global::log::clear_gl_errors();

  logger.trace(fmt::sprintf("getting uniform '%s' location.", name));
  GLint const loc = glGetUniformLocation(p, name);
  logger.trace(fmt::sprintf("uniform '%s' found at '%d'.", name, loc));

  check_opengl_errors(logger, p);
  assert(-1 != loc);
  return loc;
}

} // ns impl

template <typename L>
void
program_set_uniform_matrix_4fv(L &logger, GLuint const p, GLchar const *name, glm::mat4 const &matrix)
{
  auto const loc = impl::get_uniform_location(logger, p, name);

  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // count:
  // For the matrix (glUniformMatrix*) commands, specifies the number of matrices that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array of matrices, and 1 or more
  // if it is an array of matrices.
  GLsizei constexpr COUNT = 1;
  GLboolean constexpr TRANSPOSE_MATRICES = GL_FALSE;

  logger.trace(fmt::sprintf("sending uniform matrix at loc '%d' with data '%s' to GPU", loc,
        glm::to_string(matrix)));
  glUniformMatrix4fv(loc, COUNT, TRANSPOSE_MATRICES, glm::value_ptr(matrix));
  check_opengl_errors(logger, p);
}

template <typename L>
void
program_set_uniform_array_4fv(L &logger, GLuint const p, GLchar const *name, std::array<float, 4> const &floats)
{
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // For the vector (glUniform*v) commands, specifies the number of elements that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
  // array.
  GLsizei constexpr COUNT = 1;

  auto const loc = impl::get_uniform_location(logger, p, name);
  glUniform4fv(loc, COUNT, floats.data());
  check_opengl_errors(logger, p);
}

struct program_factory {
  program_factory() = delete;

  static stlw::result<GLuint, std::string>
  from_files(vertex_shader_filename const, fragment_shader_filename const);

  static GLuint make_invalid() { return INVALID_PROGRAM_ID(); }
};

} // ns gfx::opengl
