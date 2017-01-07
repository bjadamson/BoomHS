#pragma once
#include <opengl/global.hpp>
#include <opengl/render_shape.hpp>

namespace opengl::render
{

namespace impl
{

template<typename L, typename P, typename SHAPE>
void
render_2d_impl(L &logger, P &pipeline, SHAPE const& shape)
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
    LOG_TRACE("before drawing shape ...");
    render::render_shape(logger, shape);

    program.check_errors(logger);
    LOG_TRACE("after drawing shape");
  };

  render::draw_scene(logger, pipeline, fn, shape);
}

template <typename L, typename P, typename SHAPE>
void
render_2dscene(L &logger, P &pipeline, SHAPE const &shape)
{
  auto const fn = [&]() {
    render_2d_impl(logger, pipeline, shape);
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

template<typename Args, typename P, typename SHAPE>
void draw2d(Args const& args, P &pipeline, SHAPE const& shape)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    impl::render_2dscene(args.logger, pipeline, gl_mapped_shapes);
  };
  draw_shape(fn, shape);
}

} // ns opengl::render
