#pragma once
#include <gfx/opengl/context.hpp>
#include <gfx/opengl/shader_program.hpp>
#include <gfx/opengl/vertex_attribute.hpp>
#include <stlw/type_macros.hpp>

namespace gfx::opengl
{

template<typename C>
class pipeline
{
  shader_program program_;
  //opengl_context context;
  C context_;
  vertex_attribute va_;
public:

  //explicit pipeline(shader_program &&sp, opengl_context &&ctx, vertex_attribute &&v)
  explicit pipeline(shader_program &&sp, C &&ctx, vertex_attribute &&v)
    : program_(MOVE(sp))
    , context_(MOVE(ctx))
    , va_(MOVE(v))
  {
  }

  auto const& va() const { return this->va_; }
  auto const& ctx() const { return this->context_; }
  auto& program_ref() { return this->program_; }

  using CTX = C;
  MOVE_CONSTRUCTIBLE_ONLY(pipeline);
};

template<typename C>
auto
make_pipeline(shader_program &&sp, C &&context, vertex_attribute &&va)
{
  return pipeline<C>{MOVE(sp), MOVE(context), MOVE(va)};
}

} // ns gfx::opengl
