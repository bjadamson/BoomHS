#pragma once
#include <opengl/gfx_to_opengl.hpp>
#include <opengl/global.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace opengl::render
{

namespace impl
{

template <typename L>
void
render(L &logger, GLenum const render_mode, GLsizei const element_count)
{
  auto const fmt = fmt::sprintf("glDrawElements() render_mode '%d', element_count '%d'",
                                render_mode, element_count);

  LOG_TRACE(fmt);

  auto constexpr OFFSET = nullptr;
  glDrawElements(render_mode, element_count, GL_UNSIGNED_INT, OFFSET);
}

template <typename L, typename S>
void
log_shape_bytes(L &logger, S const &shape)
{
  assert(0 < shape.vertices.size());

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
                          shape.vertices.size(), vertices_size_in_bytes(shape));
  ostream << "data(bytes):\n";
  print(ostream, shape.vertices.size(), shape.vertices.data());

  ostream << fmt::sprintf("ordering_count %d, ordering_size_in_bytes %d\n", shape.ordering.size(),
                          ordering_size_in_bytes(shape));
  ostream << "ordering(bytes):\n";
  print(ostream, shape.ordering.size(), shape.ordering.data());
  LOG_TRACE(ostream.str());
}

template <typename L, typename S>
void
copy_to_gpu(L &logger, S const &shape)
{
  log_shape_bytes(logger, shape);

  // copy the vertices
  glBufferData(GL_ARRAY_BUFFER, vertices_size_in_bytes(shape), shape.vertices.data(),
               GL_STATIC_DRAW);

  // copy the vertice rendering order
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, ordering_size_in_bytes(shape), shape.ordering.data(),
               GL_STATIC_DRAW);
}

} // ns impl

template <typename L, typename S>
void
render_shape(L &logger, S const &shape)
{
  LOG_TRACE(fmt::sprintf("%-15s %-15s %-15s\n", "num bytes", "num floats", "num vertices"));
  LOG_TRACE(fmt::sprintf("%-15d %-15d %-15d\n", vertices_size_in_bytes(shape),
                            shape.vertices.size(), vertice_count(shape)));

  // print_triangle(logger, t0);
  impl::copy_to_gpu(logger, shape);

  // Draw our first triangle
  impl::render(logger, shape.draw_mode, shape.ordering.size());
}

template <typename L, typename P, typename FN, typename SHAPE>
void
draw_scene(L &logger, P &pipeline, FN const& fn, SHAPE const &shape)
{
  auto &program = pipeline.program_ref();
  program.use();
  program.check_errors(logger);

  using C = typename P::CTX;
  if constexpr (C::HAS_COLOR_UNIFORM) {
    auto const& ctx = pipeline.ctx();
    program.set_uniform_array_4fv(logger, "u_color", ctx.color());
    program.check_errors(logger);
  }

  // Instruct the vertex-processor to enable the vertex attributes for this context.
  global::set_vertex_attributes(logger, pipeline.va());
  fn(shape);
}

template <typename FN, typename SHAPE>
void draw_shape(FN const& fn, SHAPE const& shape)
{
  auto const gl_mapped_shapes = shape_mapper::map_to_opengl(shape);
  fn(gl_mapped_shapes);
}

} // ns opengl::render
