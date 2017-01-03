#pragma once
#include <gfx/opengl/global.hpp>
#include <gfx/opengl/render_shape.hpp>

namespace gfx::opengl::render
{

namespace impl
{

template<typename L, typename P, typename B>
void
render_2d_impl(L &logger, P &pipeline, B const& burrito)
{
  auto const& ctx = pipeline.ctx();
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  auto &program = pipeline.program_ref();
  program.use();
  auto const fn = [&](auto const &shape) {
    logger.trace("before drawing shape ...");
    render::render_shape(logger, shape);

    program.check_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, pipeline, fn, burrito);
}

template <typename L, typename P, typename B>
void
render_2dscene(L &logger, P &pipeline, B const &burrito)
{
  auto const fn = [&]() {
    render_2d_impl(logger, pipeline, burrito);
  };

  using C = typename P::CTX;
  if constexpr (C::HAS_TEXTURE) {
    auto const& ctx = pipeline.ctx();
    global::texture_bind(ctx.texture());
    ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });
    fn();
  } else {
    fn();
  }
}

} // ns impl

template<typename Args, typename P, typename B>
void draw2d(Args const& args, P &pipeline, B const& burrito)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    impl::render_2dscene(args.logger, pipeline, gl_mapped_shapes);
  };
  draw_shapes(fn, burrito);
}

} // ns gfx::opengl::render
