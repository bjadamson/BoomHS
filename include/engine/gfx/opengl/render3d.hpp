#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/render.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace engine::gfx::opengl::render3d
{

namespace impl
{

template <typename L, typename C, typename... S>
void
draw_scene(L &logger, C &ctx, glm::mat4 const& view, glm::mat4 const& projection,
    std::tuple<S...> const& shapes)
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
    logger.trace("setting u_mvmatrix");
    auto const& model = shape.model();

    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    p.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    logger.trace("before drawing shape ...");
    render::render_shape(logger, ctx, shape);
    p.check_opengl_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, ctx, fn, shapes);
}

} // ns impl

template <typename L, typename... S>
void
draw_scene(L &logger, color3d_context &ctx, glm::mat4 const &view,
           glm::mat4 const &projection, std::tuple<S...> const &shapes)
{
  impl::draw_scene(logger, ctx, view, projection, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, texture3d_context &ctx, glm::mat4 const &view,
           glm::mat4 const &projection, std::tuple<S...> const &shapes)
{
  global::texture_bind(ctx.texture());
  ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, opengl_wireframe_context &ctx, glm::mat4 const &view,
           glm::mat4 const &projection, std::tuple<S...> const &shapes)
{
  impl::draw_scene(logger, ctx, projection, shapes);
}

} // ns engine::gfx::opengl::render3d
