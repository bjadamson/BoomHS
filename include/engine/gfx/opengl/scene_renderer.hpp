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

auto const print_triangle = [](auto &logger, auto const &triangle) {
  auto const make_part = [&](float const *d) {
    // clang-format off
    auto const fmt_s =
      "verts : [{0}, {1}, {2}, {3}], "
      "colors: [{4}, {5}, {6}, {7}], "
      "tcords: [{8}, {9}]";
    return fmt::format(fmt_s,
        d[0], d[1], d[2], d[3],
        d[4], d[5], d[6], d[7],
        d[8], d[9]);
    // clang-format on
  };
  auto const p0 = make_part(&triangle.data()[0]);
  auto const p1 = make_part(&triangle.data()[10]);
  auto const p2 = make_part(&triangle.data()[20]);

  auto const msg = fmt::format("triangle info:\n{}\n{}\n{}\n\n", p0, p1, p2);
  logger.info(msg);
};

template <typename L>
void
render(GLenum const render_mode, GLsizei const vertice_count, L &logger)
{
  GLint const begin = 0;
  glDrawArrays(render_mode, begin, vertice_count);
}

template <typename L, typename S>
void
copy_to_gpu(L &logger, GLuint const vbo, S const &shape)
{
  // 1. Bind the vbo object to the GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // 2. Setup temporary object to unbind the GL_ARRAY_BUFFER from a vbo on scope exit.
  ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

  // 3. Copy the data to the GPU, then let stack cleanup gl context automatically.
  auto const log_bytes = [](auto &logger, auto const &t) {
    std::string fmt = fmt::sprintf("[[ size: %d]", t.size_in_bytes());
    for (auto i = 0; i < t.size_in_bytes(); ++i) {
      fmt += " " + std::to_string(t.data()[i]);
    }
    fmt += "]";
    logger.info(fmt);
  };
  log_bytes(logger, shape);
  glBufferData(GL_ARRAY_BUFFER, shape.size_in_bytes(), shape.data(), GL_STATIC_DRAW);
}

template <typename L, typename S>
void
render_shape(L &logger, opengl_context &ctx, S const &shape)
{
  // print_triangle(logger, t0);
  copy_to_gpu(logger, ctx.vbo(), shape);

  // Draw our first triangle
  program &p = ctx.program_ref();
  p.use();
  p.check_opengl_errors(logger);
  render(shape.draw_mode(), shape.vertice_count(), logger);
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

  global::texture_bind(ctx.texture());
  ON_SCOPE_EXIT([]() { global::texture_unbind(); });

  // Pass the matrices to the shader
  auto &p = ctx.program_ref();
  p.set_uniform_matrix_4fv("view", view);
  p.set_uniform_matrix_4fv("projection", projection);

  // Instruct the vertex-processor to enable the vertex attributes for this context.
  global::set_vertex_attributes(logger, ctx.va());

  auto const fn = [&logger, &ctx](auto const &shape) { impl::render_shape(logger, ctx, shape); };
  stlw::for_each(shapes, fn);
}

} // ns engine::gfx::opengl::renderer
