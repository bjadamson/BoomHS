#pragma once
#include <stlw/type_macros.hpp>
#include <gfx/opengl/program.hpp>
#include <gfx/opengl/vertex_attribute.hpp>

namespace gfx::opengl
{

// Essentially a "handle" over the program-object (GLuint) native OpenGL provides, but adds C++
// move-semantics.
template<typename C>
class pipeline
{
  GLuint program_;
  C context_;
  vertex_attribute va_;

  explicit pipeline(GLuint &&p, C &&ctx, vertex_attribute &&va)
      : program_(MOVE(p))
      , context_(MOVE(ctx))
      , va_(MOVE(va))
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
  using CTX = C;
  pipeline(pipeline &&o)
    : program_(o.program_)
    , context_(MOVE(o.context_))
    , va_(MOVE(o.va_))
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
  auto const& va() const { return this->va_; }
  auto const& ctx() const { return this->context_; }

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

  template<typename C>
  stlw::result<pipeline<C>, std::string>
  make(vertex_shader_filename const v, fragment_shader_filename const f, C &&ctx,
      vertex_attribute &&va)
  {
    DO_TRY(auto program, program_factory::from_files(v, f));

    return pipeline<C>{MOVE(program), MOVE(ctx), MOVE(va)};
  }
};

} // ns gfx::opengl
