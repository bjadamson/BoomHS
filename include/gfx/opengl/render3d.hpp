#pragma once
#include <gfx/opengl/context.hpp>
#include <gfx/opengl/global.hpp>
#include <gfx/opengl/render.hpp>
#include <gfx/camera.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gfx::opengl::render3d
{

namespace impl
{

template <typename L, typename C, typename B>
void
draw_scene(L &logger, C &ctx, glm::mat4 const& view, glm::mat4 const& projection,
    B const& burrito)
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
    auto const& model = shape.model;

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

  render::draw_scene(logger, ctx, fn, burrito);
}

} // ns impl

template <typename L, typename C, typename B>
void
draw_scene(L &logger, C &ctx, camera const &camera, glm::mat4 const &projection, B const& burrito)
{
  auto const fn = [&]() {
    auto const view = compute_view(camera);
    impl::draw_scene(logger, ctx, view, projection, burrito);
  };
  if constexpr (C::HAS_TEXTURE) {
    global::texture_bind(ctx.texture());
    ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });
    fn();
  } else {
    fn();
  }
}

} // ns gfx::opengl::render3d
