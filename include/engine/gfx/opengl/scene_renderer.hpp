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

template <typename L>
void
render(L &logger, GLenum const render_mode, GLsizei const element_count)
{
  auto const fmt = fmt::sprintf("glDrawElements() render_mode '%d', element_count '%d'",
                                render_mode, element_count);

  logger.trace(fmt);

  auto constexpr OFFSET = nullptr;
  glDrawElements(render_mode, element_count, GL_UNSIGNED_INT, OFFSET);
}

template <typename L, typename S>
void
log_shape_bytes(L &logger, S const &shape)
{
  assert(0 < shape.vertices_length());

  auto const print = [](auto &ostream, auto const length, auto const *data) {
    auto i{0};
    ostream << "[";
    ostream << std::to_string(data[i++]);

    for (; i < length; ++i) {
      ostream << ", " << std::to_string(data[i]);
    }
    ostream << "]";
    ostream << "\n";
  };

  std::stringstream ostream;
  ostream << fmt::sprintf("vertices_length %d, vertices_size_in_bytes %d\n",
                          shape.vertices_length(), shape.vertices_size_in_bytes());
  ostream << "data(bytes):\n";
  print(ostream, shape.vertices_length(), shape.vertices_data());

  ostream << fmt::sprintf("ordering_count %d, ordering_size_in_bytes %d\n", shape.ordering_count(),
                          shape.ordering_size_in_bytes());
  ostream << "elements(bytes):\n";
  print(ostream, shape.ordering_count(), shape.ordering_data());
  logger.trace(ostream.str());
}

template <typename L, typename S>
void
copy_to_gpu(L &logger, S const &shape)
{
  log_shape_bytes(logger, shape);

  // copy the vertices
  glBufferData(GL_ARRAY_BUFFER, shape.vertices_size_in_bytes(), shape.vertices_data(),
               GL_STATIC_DRAW);

  // copy the vertice rendering order
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, shape.ordering_size_in_bytes(), shape.ordering_data(),
               GL_STATIC_DRAW);
}

template <typename L, typename S>
void
render_shape(L &logger, opengl_context &ctx, S const &shape)
{
  logger.trace(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
  logger.trace(fmt::sprintf("%-15d %-15d %-15d\n", shape.vertices_size_in_bytes(),
                            shape.vertices_length(), shape.vertice_count()));

  // print_triangle(logger, t0);
  copy_to_gpu(logger, shape);

  // Draw our first triangle
  render(logger, shape.draw_mode(), shape.ordering_count());
}

template <typename L, typename C, typename... S>
void
draw_scene(L &logger, C &ctx, glm::mat4 const &view, glm::mat4 const &projection,
           std::tuple<S...> const &shapes)
{
  // Pass the matrices to the shader
  auto &p = ctx.program_ref();
  logger.trace("setting u_view");
  p.set_uniform_matrix_4fv(logger, "u_view", view);
  p.check_opengl_errors(logger);

  logger.trace("setting u_projection");
  p.set_uniform_matrix_4fv(logger, "u_projection", projection);
  p.check_opengl_errors(logger);

  logger.trace("using p");
  p.use();
  p.check_opengl_errors(logger);

  // Instruct the vertex-processor to enable the vertex attributes for this context.
  global::set_vertex_attributes(logger, ctx.va());

  std::stringstream ss;
  ss << "#######################################################################################\n";
  ss << "Copying '" << sizeof...(S) << "' shapes from CPU -> OpenGL driver ...\n";

  auto const fn = [&logger, &ctx, &p](auto const &shape) {

    logger.trace("setting u_mvmatrix");
    p.set_uniform_matrix_4fv(logger, "u_mvmatrix", shape.mvmatrix().data());

    logger.trace("before drawing shape ...");
    impl::render_shape(logger, ctx, shape);
    p.check_opengl_errors(logger);
    logger.trace("after drawing shape");
  };
  stlw::for_each(shapes, fn);
  ss << "#######################################################################################\n";

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

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, opengl_texture_context &ctx, glm::mat4 const &view,
           glm::mat4 const &projection, std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  global::texture_bind(ctx.texture());
  ON_SCOPE_EXIT([&ctx]() { global::texture_unbind(ctx.texture()); });

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

template <typename L, typename... S>
void
draw_scene(L &logger, opengl_wireframe_context &ctx, glm::mat4 const &view,
           glm::mat4 const &projection, std::tuple<S...> const &shapes)
{
  global::vao_bind(ctx.vao());
  ON_SCOPE_EXIT([]() { global::vao_unbind(); });

  glBindBuffer(GL_ARRAY_BUFFER, ctx.vbo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.ebo());
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); });

  auto &p = ctx.program_ref();
  logger.trace("setting u_color");
  p.set_uniform_array_4fv(logger, "u_color", ctx.color());
  p.check_opengl_errors(logger);

  impl::draw_scene(logger, ctx, view, projection, shapes);
}

} // ns engine::gfx::opengl::renderer
