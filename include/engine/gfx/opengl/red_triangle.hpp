#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <stlw/print.hpp>
#include <stlw/type_macros.hpp>

#include <cstring> // memcpy
#include <vector>

namespace engine
{
namespace gfx
{
namespace opengl
{

auto const global_vao_bind = [](auto const vao) { glBindVertexArray(vao); };
auto const global_vao_unbind = []() { glBindVertexArray(0); };

auto const check_opengl_errors = [](auto const program_id) {
  char buffer[2096];
  int actual_length = 0;
  glGetProgramInfoLog(program_id, 2096, &actual_length, buffer);
  if (0 < actual_length) {
    std::cerr << "log: '" << std::to_string(buffer[0]) << "'\n";
  }
};

auto const print_matrix = [](auto const &matrix, int const total, int const num_per_row) {
  for (auto i = 0; i < total; ++i) {
    if ((i > 0) && ((i % num_per_row) == 0)) {
      std::cerr << "\n";
    }
    std::cerr << " "
              << "[" << std::to_string(matrix[i]) << "]";
  }
  std::cerr << "\n" << std::endl;
};

class red_triangle
{
  GLuint vao_ = 0, vbo_ = 0;
  program_handle program_handle_; // init in ctor.

  friend class factory;

private:
  static auto constexpr NUM_BUFFERS = 1;

  red_triangle(program_handle ph)
      : program_handle_(std::move(ph))
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
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
  }

  void render(program_handle const &program_handle)
  {
    // Draw our first triangle
    GLuint const program_id = program_handle.get();
    glUseProgram(program_id);
    check_opengl_errors(program_id);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  void draw(GLfloat const v0[12], GLfloat const v1[12])
  {
    auto const send_vertices_gpu = [](auto const &vbo, auto const &vinfo) {
      // 1. Bind the vbo object to the GL_ARRAY_BUFFER
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      // 2. Setup temporary object to unbind the GL_ARRAY_BUFFER from a vbo on scope exit.
      ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

      // 3. Copy the data to the GPU, then let stack cleanup gl context automatically.
      glBufferData(GL_ARRAY_BUFFER, vinfo.size_in_bytes(), vinfo.buffer(), GL_STATIC_DRAW);
    };

    struct vertices_info {
      std::vector<GLfloat> data;

      vertices_info(GLfloat const *vb, GLfloat const cb[4])
      {
        static auto constexpr FLOATS_PV = 4; // x, y, z, w
        static auto constexpr COLORS_PV = 4; // r, g, b, a

        auto const grab_v = [&](auto *buffer, int const index) {
          auto const row_offset = FLOATS_PV * index;
          for (auto i = 0; i < FLOATS_PV; ++i) {
            auto const pos = row_offset + i;
            this->data.push_back(buffer[pos]);
          }
        };
        auto const grab_c = [&](auto *buffer) {
          for (auto i = 0; i < COLORS_PV; ++i) {
            this->data.push_back(buffer[i]);
          }
        };

        for (auto i = 0; i < 3; ++i) {
          grab_v(vb, i);
          grab_c(cb);
        }
      }

      int size_in_bytes() const { return this->data.size() * sizeof(this->data[0]); }
      GLfloat const *buffer() const { return this->data.data(); }
    };

    global_vao_bind(this->vao_);
    ON_SCOPE_EXIT([]() { global_vao_unbind(); });

    {
      GLfloat const c0[4] = {0.25f, 0.50f, 0.75f, 1.00f};
      vertices_info const v0_info{v0, c0};
      send_vertices_gpu(this->vbo_, v0_info);
      render(this->program_handle_);
    }

    {
      GLfloat const c1[4] = {0.75f, 0.20f, 0.00f, 1.00f};
      vertices_info const v1_info{v1, c1};
      send_vertices_gpu(this->vbo_, v1_info);
      render(this->program_handle_);
    }
  }
};

} // opengl
} // ns gfx
} // ns engine
