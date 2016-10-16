#pragma once
#include <SOIL.h>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/shape_map.hpp>
#include <glm/glm.hpp>
#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/print.hpp>
#include <stlw/type_macros.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

auto const global_vao_bind = [](auto const vao) { glBindVertexArray(vao); };
auto const global_vao_unbind = []() { glBindVertexArray(0); };

auto const global_texture_bind = [](auto const tid) { glBindTexture(GL_TEXTURE_2D, tid); };
auto const global_texture_unbind = []() { glBindTexture(GL_TEXTURE_2D, 0); };

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

auto const load_texture = [](char const *path) {
  GLuint texture;
  glGenTextures(1, &texture);

  global_texture_bind(texture);
  ON_SCOPE_EXIT([]() { global_texture_unbind(); });

  // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int w = 0, h = 0;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
  if (nullptr == pimage) {
    std::cerr << "image didn't load.";
    std::abort();
  }
  ON_SCOPE_EXIT([&]() { SOIL_free_image_data(pimage); });

  // actually send the texture to the GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);
  glGenerateMipmap(GL_TEXTURE_2D);

  return texture;
};

class renderer
{
  GLuint vao_ = 0, vbo_ = 0;
  program program_;

  // TODO: consider moving elsewhere
  GLuint texture_ = 0;
  // TODO:end

  friend class factory;

private:
  static auto constexpr NUM_BUFFERS = 1;

  renderer(program p)
      : program_(std::move(p))
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
    ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

    this->texture_ = load_texture("assets/container.jpg");
  }

  NO_COPY(renderer);
  renderer &operator=(renderer &&) = delete;

public:
  // move-construction OK.
  renderer(renderer &&other)
      : vao_(other.vao_)
      , vbo_(other.vbo_)
      , program_(std::move(other.program_))
      , texture_(other.texture_)
  {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.program_ = program::make_invalid();
    other.texture_ = 0;
  }

  ~renderer()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  template <typename L>
  void render(GLsizei const vertice_count, L &logger, program &program)
  {
    // Draw our first triangle
    program.use();
    program.check_opengl_errors(logger);

    GLint const begin = 0;
    //glDrawArrays(GL_TRIANGLES, begin, vertice_count);
    glDrawArrays(GL_QUADS, begin, vertice_count);
  }

  template <typename L, typename S>
  void send_data_gpu(L &logger, GLuint const vbo, S const &shape)
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
  void draw(L &logger, glm::mat4 const &view, glm::mat4 const &projection, S const &t0, S const &t1)
  {
    global_vao_bind(this->vao_);
    ON_SCOPE_EXIT([]() { global_vao_unbind(); });

    global_texture_bind(this->texture_);
    ON_SCOPE_EXIT([]() { global_texture_unbind(); });

    // Pass the matrices to the shader
    this->program_.set_uniform_matrix_4fv("view", view);
    this->program_.set_uniform_matrix_4fv("projection", projection);

    print_triangle(logger, t0);
    send_data_gpu(logger, this->vbo_, t0);
    render(t0.vertice_count(), logger, this->program_);

    send_data_gpu(logger, this->vbo_, t1);
    render(t0.vertice_count(), logger, this->program_);
  }
};

} // opengl
} // ns gfx
} // ns engine
