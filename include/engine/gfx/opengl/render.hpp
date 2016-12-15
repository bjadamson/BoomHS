#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <stlw/log.hpp>
#include <stlw/print.hpp>
#include <stlw/tuple.hpp>
#include <stlw/type_macros.hpp>
#include <sstream>

namespace engine::gfx::opengl::render
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

template <typename L, typename Ctx, typename S>
void
render_shape(L &logger, Ctx &ctx, S const &shape)
{
  logger.trace(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
  logger.trace(fmt::sprintf("%-15d %-15d %-15d\n", shape.vertices_size_in_bytes(),
                            shape.vertices_length(), shape.vertice_count()));

  // print_triangle(logger, t0);
  copy_to_gpu(logger, shape);

  // Draw our first triangle
  render(logger, shape.draw_mode(), shape.ordering_count());
}

template <typename L, typename C, typename FN, typename... S>
void
draw_scene(L &logger, C &ctx, FN const& fn, std::tuple<S...> const &shapes)
{
  // Pass the matrices to the shader
  auto &p = ctx.program_ref();

  logger.trace("using p");
  p.use();
  p.check_opengl_errors(logger);

  // Instruct the vertex-processor to enable the vertex attributes for this context.
  global::set_vertex_attributes(logger, ctx.va());

  std::stringstream ss;
  ss << "#######################################################################################\n";
  ss << "Copying '" << sizeof...(S) << "' shapes from CPU -> OpenGL driver ...\n";

  stlw::for_each(shapes, fn);
  ss << "#######################################################################################\n";

  logger.trace(ss.str());
}

} // ns engine::gfx::opengl::render
