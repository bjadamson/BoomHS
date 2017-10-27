#pragma once
#include <opengl/gl_log.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/vertex_attribute.hpp>

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

namespace opengl
{

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);

#undef DEFINE_SHADER_FILENAME_TYPE

namespace impl
{

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
    LOG_ERROR("Opengl error: '{}'", *errors);
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

template <typename L>
auto
get_uniform_location(L &logger, GLuint const p, GLchar const *name)
{
  global::log::clear_gl_errors();

  LOG_TRACE(fmt::sprintf("getting uniform '%s' location.", name));
  GLint const loc = glGetUniformLocation(p, name);
  LOG_TRACE(fmt::sprintf("uniform '%s' found at '%d'.", name, loc));

  check_opengl_errors(logger, p);
  assert(-1 != loc);
  return loc;
}

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

  LOG_TRACE(fmt::sprintf("sending uniform matrix at loc '%d' with data '%s' to GPU", loc,
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

} // ns impl

struct program_factory {
  program_factory() = delete;

  static stlw::result<GLuint, std::string>
  from_files(vertex_shader_filename const, fragment_shader_filename const);

  static GLuint make_invalid() { return impl::INVALID_PROGRAM_ID(); }
};

// Essentially a "handle" over the program-object (GLuint) native OpenGL provides, but adds C++
// move-semantics.
class shader_program
{
  GLuint program_;
  explicit shader_program(GLuint &&p)
      : program_(MOVE(p))
  {
  }

  static void
  destroy(shader_program &p)
  {
    impl::destroy_program(p.program_);
    p.program_ = 0;
  }

  friend struct shader_program_factory;
  NO_COPY(shader_program);
  NO_MOVE_ASSIGN(shader_program);
public:
  shader_program(shader_program &&o)
    : program_(o.program_)
  {
    // We don't want to destroy the underlying program, we want to transfer the ownership to this
    // instance being moved into. This implements "handle-passing" allowing the user to observe
    // move-semantics for this object.
    o.program_ = program_factory::make_invalid();
  }

  ~shader_program()
  {
    if (impl::is_invalid(this->program_)) {
      destroy(*this);
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // MUTATION
  void use()
  {
    impl::use_program(this->program_);
  }

  template <typename L>
  void set_uniform_matrix_4fv(L &logger, GLchar const *name, glm::mat4 const &matrix)
  {
    use();
    impl::program_set_uniform_matrix_4fv(logger, this->program_, name, matrix);
  }

  template <typename L>
  void set_uniform_array_4fv(L &logger, GLchar const *name, std::array<float, 4> const &floats)
  {
    use();
    impl::program_set_uniform_array_4fv(logger, this->program_, name, floats);
  }

  template <typename L>
  void check_errors(L &logger)
  {
    use();
    impl::check_opengl_errors(logger, this->program_);
  }
};

struct shader_program_factory
{
  shader_program_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(shader_program_factory);

  stlw::result<shader_program, std::string>
  make(vertex_shader_filename const v, fragment_shader_filename const f) const
  {
    DO_TRY(auto program, program_factory::from_files(v, f));
    return shader_program{MOVE(program)};
  }
};

} // ns opengl
