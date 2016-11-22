#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <glm/glm.hpp>
#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/print.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/tuple.hpp>

namespace engine::gfx::opengl
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

class polygon_renderer
{
  friend class factory;

  NO_COPY(polygon_renderer);
  NO_MOVE_ASSIGN(polygon_renderer);
public:
  polygon_renderer() = default;
  MOVE_CONSTRUCTIBLE(polygon_renderer);
private:

  template <typename L>
  void render(GLenum const render_mode, GLsizei const vertice_count, L &logger,
      opengl_context &ctx)
  {
    program &program = ctx.program_ref();

    // Draw our first triangle
    program.use();
    program.check_opengl_errors(logger);

    GLint const begin = 0;
    glDrawArrays(render_mode, begin, vertice_count);
  }

  template <typename L, typename S>
  void send_data_gpu(L &logger, opengl_context const& ctx, S const &shape)
  {
    // 1. Bind the vbo object to the GL_ARRAY_BUFFER
    GLuint const vbo = ctx.vbo();
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

  template<typename L, typename S>
  void draw_shape(L &logger, opengl_context &ctx, S const& shape)
  {
    //print_triangle(logger, t0);
    send_data_gpu(logger, ctx, shape);
    render(shape.draw_mode(), shape.vertice_count(), logger, ctx);
  }

  template<typename L, typename ...S>
  void draw_shapes(L &logger, opengl_context &ctx, std::tuple<S...> const& shapes)
  {
    auto const fn = [this, &logger, &ctx] (auto const& shape) { this->draw_shape(logger, ctx, shape); };
    stlw::for_each(shapes, fn);
  }

public:
  template <typename L, typename ...S>
  void draw(L &logger, opengl_context &ctx, glm::mat4 const &view, glm::mat4 const &projection,
      std::tuple<S...> const& shapes)
  {
    global::vao_bind(ctx.vao());
    ON_SCOPE_EXIT([]() { global::vao_unbind(); });

    // Pass the matrices to the shader
    auto &p = ctx.program_ref();
    p.set_uniform_matrix_4fv("view", view);
    p.set_uniform_matrix_4fv("projection", projection);

    draw_shapes(logger, ctx, shapes);
  }
};

} // ns engine::gfx::opengl
