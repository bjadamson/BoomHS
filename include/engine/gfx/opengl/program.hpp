#pragma once
#include <engine/gfx/opengl/gl_log.hpp>
#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/global.hpp>

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

namespace engine::gfx::opengl
{

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);

class program
{
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // non-const to allow move-assignment.
  GLuint program_id_;

  // https://www.opengl.org/sdk/docs/man2/xhtml/glDeleteProgram.xml
  // from the docs:
  // A value of 0 for program will be silently ignored.

  // This means that if we assign program_id_ to 0 in the move
  // constructor/assignment operator,
  // this class can exhibit correct move semantics.
  static auto constexpr INVALID_PROGRAM_ID = 0;

  inline void destroy() { glDeleteProgram(this->program_id_); }

  GLuint get() const { return this->program_id_; }

  NO_COPY(program)

  static inline void invalidate_other_shader(program &other)
  {
    other.program_id_ = INVALID_PROGRAM_ID;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // constructors
  explicit program(GLuint const p)
      : program_id_(p)
  {
  }

public:
  ~program() { destroy(); }

  // Allow instances to be moved around freely, always taking care to invalidate the moved-from
  // instance.
  program(program &&other)
      : program_id_(other.program_id_)
  {
    invalidate_other_shader(other);
  }

  // Move-assignment implementation.
  program &operator=(program &&other)
  {
    this->program_id_ = other.program_id_;
    invalidate_other_shader(other);
    return *this;
  }

  // MUTATION
  // ----------------------------------------------------------------------------------------------
  // Make this program become "Active" for OpenGL.
  void use() { glUseProgram(this->program_id_); }

  template <typename L>
  auto get_uniform_location(L &logger, GLuint program, GLchar const *name)
  {
    global::log::clear_gl_errors();
    this->use();

    logger.trace(fmt::sprintf("getting uniform '%s' location.", name));
    GLint const loc = glGetUniformLocation(this->program_id_, name);
    logger.trace(fmt::sprintf("uniform '%s' found at '%d'.", name, loc));

    this->check_opengl_errors(logger);
    assert(-1 != loc);
    return loc;
  }

  template <typename L>
  void set_uniform_matrix_4fv(L &logger, GLchar const *name, glm::mat4 const &matrix)
  {
    auto const loc = this->get_uniform_location(logger, this->program_id_, name);

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
    this->check_opengl_errors(logger);
  }

  template <typename L>
  void set_uniform_array_4fv(L &logger, GLchar const *name, std::array<float, 4> const &floats)
  {
    // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
    //
    // For the vector (glUniform*v) commands, specifies the number of elements that are to be
    // modified.
    // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
    // array.
    GLsizei constexpr COUNT = 1;

    auto const loc = this->get_uniform_location(logger, this->program_id_, name);
    glUniform4fv(loc, COUNT, floats.data());
    this->check_opengl_errors(logger);
  }

  // IMMUTABLE
  // ----------------------------------------------------------------------------------------------
  inline std::string shader_log() const { return global::log::get_shader_log(this->program_id_); }

  inline std::string program_log() const { return global::log::get_program_log(this->program_id_); }

  template <typename L>
  inline void check_opengl_errors(L &logger) const
  {
    auto const errors = global::log::get_errors(this->program_id_);
    if (errors) {
      logger.error("Opengl error: '{}'", *errors);
    }
  }

  // Factory functions
  static program make(GLuint const program_id) { return program{program_id}; }

  static program make_invalid() { return make(INVALID_PROGRAM_ID); }
};

struct program_loader {
  program_loader() = delete;

  static stlw::result<program, std::string>
  from_files(vertex_shader_filename const, fragment_shader_filename const);
};

} // ns engine::gfx::opengl
