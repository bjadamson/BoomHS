#pragma once
#include <gfx/opengl/global.hpp>
#include <gfx/opengl/render_shape.hpp>
#include <gfx/camera.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gfx::opengl::render
{

namespace impl
{

template <typename L, typename P, typename B>
void
draw_scene(L &logger, P &pipeline, glm::mat4 const& view, glm::mat4 const& projection,
    B const& burrito)
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
    logger.trace("setting u_mvmatrix");
    auto const& model = shape.model;

    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    pipeline.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    logger.trace("before drawing shape ...");
    render::render_shape(logger, shape);
    pipeline.check_errors(logger);
    logger.trace("after drawing shape");
  };

  render::draw_scene(logger, pipeline, fn, burrito);
}

template <typename L, typename P, typename B>
void
draw_scene(L &logger, P &pipeline, camera const &camera, glm::mat4 const &projection, B const& burrito)
{
  auto const fn = [&]() {
    auto const view = compute_view(camera);
    impl::draw_scene(logger, pipeline, view, projection, burrito);
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

template <typename Args, typename P, typename B>
void draw3d(Args const &args, P &pipeline, B const& burrito)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    impl::draw_scene(args.logger, pipeline, args.camera, args.projection, gl_mapped_shapes);
  };
  render::draw_shapes(fn, burrito);
}

} // ns gfx::opengl::render
