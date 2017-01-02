#pragma once
#include <gfx/opengl/program.hpp>
#include <stlw/type_macros.hpp>

namespace gfx::opengl
{

// Essentially a "handle" over the program-object (GLuint) native OpenGL provides, but adds C++
// move-semantics.
class pipeline
{
  GLuint program_;

  explicit pipeline(GLuint &&p)
      : program_(MOVE(p))
  {
  }

  static void
  destroy(pipeline &p)
  {
    destroy_program(p.program_);
    p.program_ = 0;
  }

  friend class pipeline_factory;
  NO_COPY(pipeline);
  NO_MOVE_ASSIGN(pipeline);
public:
  pipeline(pipeline &&o) : program_(o.program_)
  {
    // We don't want to destroy the underlying program, we want to transfer the ownership to this
    // instance being moved into. This implements "handle-passing" allowing the user to observe
    // move-semantics for this object.
    o.program_ = program_factory::make_invalid();
  }

  ~pipeline()
  {
    if (is_invalid(this->program_)) {
      destroy(*this);
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // MUTATION
  void use()
  {
    use_program(this->program_);
  }

  template <typename L>
  void set_uniform_matrix_4fv(L &logger, GLchar const *name, glm::mat4 const &matrix)
  {
    use();
    program_set_uniform_matrix_4fv(logger, this->program_, name, matrix);
  }

  template <typename L>
  void set_uniform_array_4fv(L &logger, GLchar const *name, std::array<float, 4> const &floats)
  {
    use();
    program_set_uniform_array_4fv(logger, this->program_, name, floats);
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // IMMUTABLE
  template <typename L>
  inline void check_errors(L &logger)
  {
    use();
    check_opengl_errors(logger, this->program_);
  }
};

struct pipeline_factory
{
  pipeline_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(pipeline_factory);

  stlw::result<pipeline, std::string>
  make(vertex_shader_filename const v, fragment_shader_filename const f)
  {
    DO_TRY(auto program, program_factory::from_files(v, f));
    return pipeline{MOVE(program)};
  }

  static pipeline make_invalid() { return pipeline{program_factory::make_invalid()}; };
};

} // ns gfx::opengl
