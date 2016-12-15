#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/render.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace engine::gfx::opengl::render2d
{

template <typename L, typename... S>
void
draw_scene(L &logger, color2d_context &ctx, glm::mat4 const &projection,
    std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  auto &p = ctx.program_ref();
  p.use();
  logger.trace("setting u_projection");
  p.set_uniform_matrix_4fv(logger, "u_projection", projection);
  p.check_opengl_errors(logger);

  auto const fn = [&](auto const &shape) {
    logger.trace("setting u_projection");
    p.set_uniform_matrix_4fv(logger, "u_projection", projection);
    p.check_opengl_errors(logger);

    logger.trace("before drawing shape ...");
    render::render_shape(logger, ctx, shape);

    p.check_opengl_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, ctx, projection, fn, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, texture2d_context &ctx, glm::mat4 const &projection,
    std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  global::texture_bind(ctx.texture());
  ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });

  auto &p = ctx.program_ref();
  p.use();
  logger.trace("setting u_projection");
  p.set_uniform_matrix_4fv(logger, "u_projection", projection);
  p.check_opengl_errors(logger);

  auto const fn = [&](auto const &shape) {
    logger.trace("before drawing shape ...");
    render::render_shape(logger, ctx, shape);

    p.check_opengl_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, ctx, projection, fn, shapes);
}

} // ns engine::gfx::opengl::render2d
