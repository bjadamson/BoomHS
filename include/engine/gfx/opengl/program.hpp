#pragma once
#include <engine/gfx/opengl_glew.hpp>
#include <engine/gfx/opengl/gl.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/result.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

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

  // Make this program become "Active" for OpenGL.
  void use() { glUseProgram(this->program_id_); }

  inline std::string
  shader_log() const { return gl_log::get_shader_log(this->program_id_); }

  inline std::string
  program_log() const { return gl_log::get_program_log(this->program_id_); }

  template<typename L>
  inline void
  check_opengl_errors(L &logger) const
  {
    auto const errors = gl_log::get_opengl_errors(this->program_id_);
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
  load(char const *vertex_file_path, char const *fragment_file_path);
};

} // ns opengl
} // ns gfx
} // ns engine
