#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/render.hpp>

namespace engine::gfx::opengl::render2d
{

namespace impl
{

template<typename L, typename C, typename ...S>
void
draw_scene(L &logger, C &ctx, std::tuple<S...> const& shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  auto &p = ctx.program_ref();
  p.use();
  auto const fn = [&](auto const &shape) {
    logger.trace("before drawing shape ...");
    render::render_shape(logger, ctx, shape);

    p.check_opengl_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, ctx, fn, shapes);
}

} // ns impl

template <typename L, typename C, typename... S>
void
draw_scene(L &logger, C &ctx, std::tuple<S...> const &shapes)
{
  auto const fn = [&]() {
    impl::draw_scene(logger, ctx, shapes);
  };
  if constexpr (C::HAS_TEXTURE) {
    global::texture_bind(ctx.texture());
    ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });
    fn();
  } else {
    fn();
  }
}

} // ns engine::gfx::opengl::render2d
