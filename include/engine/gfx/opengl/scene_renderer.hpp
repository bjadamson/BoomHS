#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <glm/glm.hpp>
#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/print.hpp>
#include <stlw/tuple.hpp>
#include <stlw/type_macros.hpp>

namespace engine::gfx::opengl::renderer
{

namespace impl
{

template<typename L>
void
render(L &logger, GLenum const render_mode, GLsizei const vertice_count)
{
  GLint const begin = 0;
  logger.info(fmt::sprintf("glDrawArrays() begin '%d', mode '%d', vertice_count '%d'", begin,
        render_mode, vertice_count));
  glDrawArrays(render_mode, begin, vertice_count);
}

template<typename L, typename S>
void
log_shape_bytes(L &logger, S const& shape)
{
  assert(0 < shape.length());
  std::stringstream ostream;
  ostream << "data(bytes):\n";
  auto i{0};
  ostream << "[";
  ostream << std::to_string(shape.data()[i++]);

  for (; i < shape.length(); ++i) {
    ostream << ", " << std::to_string(shape.data()[i]);
  }
  ostream << "]";
  ostream << "\n";
  logger.trace(ostream.str());
}

template <typename L, typename S>
void
copy_to_gpu(L &logger, GLuint const vbo, S const& shape)
{
  // 1. Bind the vbo object to the GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // 2. Setup temporary object to unbind the GL_ARRAY_BUFFER from a vbo on scope exit.
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  // 3. Copy the data to the GPU, then let stack cleanup gl context automatically.
  log_shape_bytes(logger, shape);
  glBufferData(GL_ARRAY_BUFFER, shape.size_in_bytes(), shape.data(), GL_STATIC_DRAW);
}

template <typename L, typename S>
void
render_shape(L &logger, opengl_context &ctx, S const &shape)
{
  logger.trace(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
  logger.trace(fmt::sprintf("%-15d %-15d %-15d\n", shape.size_in_bytes(), shape.length(),
      shape.vertice_count()));

  // print_triangle(logger, t0);
  copy_to_gpu(logger, ctx.vbo(), shape);

  // Draw our first triangle
  render(logger, shape.draw_mode(), shape.vertice_count());
}

template<typename L, typename C, typename ...S>
void
draw_scene(L &logger, C &ctx, glm::mat4 const& view, glm::mat4 const& projection,
    std::tuple<S...> const& shapes)
{
  // Pass the matrices to the shader
  auto &p = ctx.program_ref();
  logger.info("setting u_view");
  p.set_uniform_matrix_4fv(logger, "u_view", view);
  p.check_opengl_errors(logger);

  logger.info("setting u_projection");
  p.set_uniform_matrix_4fv(logger, "u_projection", projection);
  p.check_opengl_errors(logger);

  logger.info("using p");
  p.use();
  p.check_opengl_errors(logger);

  // Instruct the vertex-processor to enable the vertex attributes for this context.
  global::set_vertex_attributes(logger, ctx.va());

  std::stringstream ss;
  ss << "#########################################################################################################################\n";
  ss << "Copying '" << sizeof...(S) << "' shapes from CPU -> OpenGL driver ...\n";

  auto const fn = [&logger, &ctx, &p](auto const &shape) {
    logger.trace("before drawing shape ...");
    impl::render_shape(logger, ctx, shape);
    p.check_opengl_errors(logger);
    logger.trace("after drawing shape");
  };
  stlw::for_each(shapes, fn);
  ss << "#########################################################################################################################\n";

  logger.trace(ss.str());
}

} // ns impl

template <typename L, typename... S>
void
draw_scene(L &logger, opengl_context &ctx, glm::mat4 const &view, glm::mat4 const &projection,
           std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, opengl_texture_context &ctx, glm::mat4 const &view, glm::mat4 const &projection,
           std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  global::texture_bind(ctx.texture());
  ON_SCOPE_EXIT([]() { global::texture_unbind(); });

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, opengl_wireframe_context &ctx, glm::mat4 const &view, glm::mat4 const &projection,
           std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  auto &p = ctx.program_ref();
  logger.info("setting u_color");
  p.set_uniform_array_4fv(logger, "u_color", ctx.color());
  p.check_opengl_errors(logger);

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

} // ns engine::gfx::opengl::renderer
