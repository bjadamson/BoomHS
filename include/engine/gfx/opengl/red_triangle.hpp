#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl_gfx.hpp>
#include <stlw/type_macros.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

auto const global_vao_bind = [](auto const vao) { glBindVertexArray(vao); };
auto const global_vao_unbind = []() { glBindVertexArray(0); };

auto const global_enable_vattrib_array = [](auto const index) {
  glEnableVertexAttribArray(index);
};

auto const check_opengl_errors = [](auto const program_id) {
  char buffer[2096];
  int actual_length = 0;
  glGetProgramInfoLog(program_id, 2096, &actual_length, buffer);
  if (0 < actual_length) {
    std::cerr << "log: '" << std::to_string(buffer[0]) << "'\n";
  }
};

class red_triangle
{
  GLuint vao_ = 0, vbo_ = 0;

public:
  program_handle program_handle_ = program_handle::make_invalid();

private:
  static auto constexpr RED_TRIANGLE_VERTEX_POSITION_INDEX = 0;
  static auto constexpr RED_TRIANGLE_VERTEX_COLOR_INDEX = 0;
  static auto constexpr NUM_BUFFERS = 1;

  red_triangle()
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
  }

  NO_COPY(red_triangle);
  red_triangle &operator=(red_triangle &&) = delete;

public:
  // move-assignment OK.
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
    auto const send_vertices_gpu =
        [](auto const &vbo, auto const vinfo) // GLfloat const vertices[12])
    {
      // 1. Bind the vbo object to the GL_ARRAY_BUFFER
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      // 2. Setup temporary object to unbind the GL_ARRAY_BUFFER from a vbo on scope exit.
      ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

      // 3. Copy the data to the GPU, then let stack cleanup gl context automatically.
      glBufferData(GL_ARRAY_BUFFER, vinfo.size_in_bytes, vinfo.buffer, GL_STATIC_DRAW);
    };

    struct vertices_info {
      int const size_in_bytes;
      GLfloat const *buffer;

      vertices_info(int const s, GLfloat const *b)
          : size_in_bytes(s)
          , buffer(b)
      {
      }
    };

    global_vao_bind(this->vao_);
    ON_SCOPE_EXIT([]() { global_vao_unbind(); });

    vertices_info const v0_info{sizeof(v0[0]) * 12, v0};
    send_vertices_gpu(this->vbo_, v0_info);
    render(this->program_handle_);

    vertices_info const v1_info{sizeof(v1[0]) * 12, v1};
    send_vertices_gpu(this->vbo_, v1_info);
    render(this->program_handle_);
  }

public:
  static red_triangle make()
  {
    red_triangle triangle;
    global_vao_bind(triangle.vao_);
    ON_SCOPE_EXIT([]() { global_vao_unbind(); });

    global_enable_vattrib_array(RED_TRIANGLE_VERTEX_POSITION_INDEX);

    static auto constexpr NUM_VERTICES = 4;
    static auto constexpr TYPE_OF_DATA = GL_FLOAT;
    static auto constexpr NORMALIZE_DATA = GL_FALSE;
    static auto constexpr STRIDE_SIZE = NUM_VERTICES * sizeof(TYPE_OF_DATA);
    static auto constexpr OFFSET_PTR = nullptr;

    glBindBuffer(GL_ARRAY_BUFFER, triangle.vbo_);
    ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

    // configure this OpenGL VAO attribute array
    glVertexAttribPointer(RED_TRIANGLE_VERTEX_POSITION_INDEX, NUM_VERTICES, TYPE_OF_DATA,
                          NORMALIZE_DATA, STRIDE_SIZE, OFFSET_PTR);
    return triangle;
  }
};

} // opengl
} // ns gfx
} // ns engine
