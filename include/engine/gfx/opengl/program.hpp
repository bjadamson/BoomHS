#pragma once
#include <engine/gfx/opengl_glew.hpp>
#include <stlw/result.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

class program_handle
{
  GLuint program_id_; // non-const to allow move-assignment.

  // https://www.opengl.org/sdk/docs/man2/xhtml/glDeleteProgram.xml
  // from the docs:
  // A value of 0 for program will be silently ignored.

  // This means that if we assign program_id_ to 0 in the move constructor/assignment operator,
  // this class can exhibit correct move semantics.
  static auto constexpr INVALID_PROGRAM_ID = 0;

  inline void
  destroy()
  {
    glDeleteProgram(this->program_id_);
  }

  static inline void
  invalidate_other_shader(program_handle &other)
  {
    other.program_id_ = INVALID_PROGRAM_ID;
  }

  explicit program_handle(GLuint const p) : program_id_(p) {}
public:
  ~program_handle() { destroy(); }

  program_handle(program_handle &&other) : program_id_(other.program_id_)
  {
    invalidate_other_shader(other);
  }

  program_handle& operator=(program_handle &&other)
  {
    this->program_id_ = other.program_id_;
    invalidate_other_shader(other);
    return *this;
  }

  // TODO: maybe don't make public, make it a friend of something that calls get() automatically.
  GLuint
  get() const { return this->program_id_; }

  static program_handle
  make(GLuint const program_id) { return program_handle{program_id}; }

  static program_handle
  make_invalid() { return make(INVALID_PROGRAM_ID); }
};

struct program_loader
{
  program_loader() = delete;

  static stlw::result<program_handle, std::string>
  load(char const* vertex_file_path, char const* fragment_file_path);
};

} // ns opengl
} // ns gfx
} // ns engine
