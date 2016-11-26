#pragma once
#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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
  GLuint program_id_; // non-const to allow move-assignment.

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

  void set_uniform_matrix_4fv(GLchar const *name, glm::mat4 const &matrix)
  {
    GLint const loc = glGetUniformLocation(this->program_id_, name);
    GLsizei const NUM_MATRICES = 1;
    GLboolean const transpose_matrices = GL_FALSE;
    glUniformMatrix4fv(loc, NUM_MATRICES, transpose_matrices, glm::value_ptr(matrix));
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
