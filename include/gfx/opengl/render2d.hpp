#pragma once
#include <gfx/opengl/context.hpp>
#include <gfx/opengl/global.hpp>
#include <gfx/opengl/render.hpp>

namespace gfx::opengl::render2d
{

namespace impl
{

template<typename L, typename P, typename B>
void
draw_scene(L &logger, P &pipeline, B const& burrito)
{
  auto const& ctx = pipeline.ctx();
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  pipeline.use();
  auto const fn = [&](auto const &shape) {
    logger.trace("before drawing shape ...");
    render::render_shape(logger, shape);

    pipeline.check_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, pipeline, fn, burrito);
}

} // ns impl

template <typename L, typename P, typename B>
void
draw_scene(L &logger, P &pipeline, B const &burrito)
{
  auto const fn = [&]() {
    impl::draw_scene(logger, pipeline, burrito);
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

} // ns gfx::opengl::render2d
