#pragma once
#include <opengl/global.hpp>
#include <opengl/render_shape.hpp>
#include <opengl/camera.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace opengl::render
{

namespace impl
{

template <typename L, typename P, typename SHAPE>
void
draw_scene(L &logger, P &pipeline, glm::mat4 const& view, glm::mat4 const& projection,
    SHAPE const& shape)
{
  auto const& ctx = pipeline.ctx();
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  auto &program = pipeline.program_ref();
  auto const fn = [&](auto const &shape) {
    LOG_TRACE("setting u_mvmatrix");
    auto const& model = shape.model;

    auto const tmatrix = glm::translate(glm::mat4{}, model.translation);
    auto const rmatrix = glm::toMat4(model.rotation);
    auto const smatrix = glm::scale(glm::mat4{}, model.scale);
    auto const mmatrix = tmatrix * rmatrix * smatrix;
    auto const mvmatrix = projection * view * mmatrix;
    program.set_uniform_matrix_4fv(logger, "u_mvmatrix", mvmatrix);

    LOG_TRACE("before drawing shape ...");
    render::render_shape(logger, shape);
    program.check_errors(logger);
    LOG_TRACE("after drawing shape");
  };

  render::draw_scene(logger, pipeline, fn, shape);
}

template <typename L, typename P, typename SHAPE>
void
draw_scene(L &logger, P &pipeline, camera const &camera, glm::mat4 const &projection, SHAPE const& shape)
{
  auto const fn = [&]() {
    auto const view = compute_view(camera);
    impl::draw_scene(logger, pipeline, view, projection, shape);
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

template <typename Args, typename P, typename SHAPE>
void draw3d(Args const &args, P &pipeline, SHAPE const& shape)
{
  auto const fn = [&](auto const& gl_mapped_shapes) {
    impl::draw_scene(args.logger, pipeline, args.camera, args.projection, gl_mapped_shapes);
  };
  render::draw_shape(fn, shape);
}

} // ns opengl::render
