#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/shape_map.hpp>
#include <stlw/format.hpp>
#include <stlw/print.hpp>
#include <stlw/type_macros.hpp>
#include <SOIL.h>

namespace engine
{
namespace gfx
{
namespace opengl
{

auto const global_vao_bind = [](auto const vao) { glBindVertexArray(vao); };
auto const global_vao_unbind = []() { glBindVertexArray(0); };

auto const check_opengl_errors = [](auto &logger, auto const program_id) {
  char buffer[2096];
  int actual_length = 0;
  glGetProgramInfoLog(program_id, 2096, &actual_length, buffer);
  if (0 < actual_length) {
    logger.error("Opengl error: '{}'", std::to_string(buffer[0]));
  }
};

auto const print_triangle = [](auto &logger, auto const &triangle) {
  auto const make_part = [&](float const* d)
  {
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

  auto const msg = stlw::format("triangle info:\n{}\n{}\n{}\n\n", p0, p1, p2);
  logger.info(msg);
};

class red_triangle
{
  GLuint vao_ = 0, vbo_ = 0;
  program_handle program_handle_; // init in ctor.

  // TODO: consider moving elsewhere
  GLuint texture_;
  int width_ = 0, height_ = 0;
  unsigned char *image_ = nullptr;
  // TODO:end

  friend class factory;
private:
  static auto constexpr NUM_BUFFERS = 1;

  red_triangle(program_handle ph)
      : program_handle_(std::move(ph))
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);

    glGenTextures(1, &this->texture_);
    glBindTexture(GL_TEXTURE_2D, this->texture_);

    // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    this->image_ = SOIL_load_image("container.jpg", &this->width_, &this->height_, 0, SOIL_LOAD_RGB);
    if (nullptr == this->image_) {
      std::cerr << "image didn't load.";
      std::abort();
    }

    // actually send the texture to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width_, this->height_, 0, GL_RGB, GL_UNSIGNED_BYTE,
        this->image_);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  NO_COPY(red_triangle);
  red_triangle &operator=(red_triangle &&) = delete;

public:
  // move-construction OK.
  red_triangle(red_triangle &&other)
      : vao_(other.vao_)
      , vbo_(other.vbo_)
      , program_handle_(std::move(other.program_handle_))
  {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.program_handle_ = program_handle::make_invalid();
  }

  ~red_triangle()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);

    SOIL_free_image_data(this->image_);
  }

  template<typename L>
  void render(L &logger, program_handle const &program_handle)
  {
    // Draw our first triangle
    GLuint const program_id = program_handle.get();
    glUseProgram(program_id);
    check_opengl_errors(logger, program_id);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  template<typename L>
  void draw(L &logger, triangle const &t0, triangle const &t1)
  {
    auto const send_vertices_gpu = [](auto const &vbo, auto const &triangle) {
      // 1. Bind the vbo object to the GL_ARRAY_BUFFER
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      // 2. Setup temporary object to unbind the GL_ARRAY_BUFFER from a vbo on scope exit.
      ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

      // 3. Copy the data to the GPU, then let stack cleanup gl context automatically.
      glBufferData(GL_ARRAY_BUFFER, triangle.size_in_bytes(), triangle.data(), GL_STATIC_DRAW);
    };

    global_vao_bind(this->vao_);
    ON_SCOPE_EXIT([]() { global_vao_unbind(); });

    glBindTexture(GL_TEXTURE_2D, this->texture_);
    ON_SCOPE_EXIT([]() { glBindTexture(GL_TEXTURE_2D, 0); });

    print_triangle(logger, t0);
    send_vertices_gpu(this->vbo_, t0);
    render(logger, this->program_handle_);

    send_vertices_gpu(this->vbo_, t1);
    render(logger, this->program_handle_);
  }
};

} // opengl
} // ns gfx
} // ns engine
