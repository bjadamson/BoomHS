#pragma once
#include <engine/gfx/opengl_gfx.hpp>
#include <engine/gfx/opengl/program.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

auto const global_bind_vao = [](auto const vao) {
  glBindVertexArray(vao);
};

auto const global_unbind_vao = [](auto const vao) {
  glBindVertexArray(0);
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

  static auto constexpr RED_TRIANGLE_VPOS = 0;
  static auto constexpr NUM_VERTICES = 4;
  static auto constexpr TYPE_OF_DATA = GL_FLOAT;
  static auto constexpr NORMALIZE_DATA = GL_FALSE;
  static auto constexpr STRIDE_SIZE = NUM_VERTICES * sizeof(TYPE_OF_DATA);
  static auto constexpr OFFSET = nullptr;

  static auto constexpr NUM_BUFFERS = 1;

  red_triangle() = default;
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

  void init_buffers()
  {
    // TODO: I think the way we're trying to encapsulate the OpenGL VAO / vertex
    // attributes is off
    // a bit, so these calls may be innapropriate for this constructor.

    // See, it might also make more sense to refactor the VAO / VBO / vertex
    // attributes into a
    // different structure all-together, that isn't in any way tied to the
    // buffer's themselves.
    //
    // This will come with experience playing with opengl I suppose.
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);

    // enable this attribute position within this VAO.
    global_bind_vao(this->vao_);

    // this just needs to called once per bound VAO.
    glEnableVertexAttribArray(0);
  }

  void destroy_buffers()
  {
    glDeleteBuffers(1, &this->vbo_);
    glDeleteVertexArrays(1, &this->vao_);
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

    global_bind_vao(this->vao_);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    global_unbind_vao(this->vao_);
  }

  void draw(GLfloat const the_vertices[12])
  {
    auto const send_vertices_gpu =
        [](auto const &vbo, auto const vinfo) // GLfloat const vertices[12])
    {
      // Bind the Vertex Array Object first, then bind and set vertex buffer(s)
      // and attribute
      // pointer(s).
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, vinfo.size_in_bytes, vinfo.buffer, GL_STATIC_DRAW);

      glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                            static_cast<GLvoid *>(nullptr));

      glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind when we are done with this scope automatically.
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

    vertices_info const vertex_info{sizeof(the_vertices[0]) * 12, the_vertices};
    send_vertices_gpu(this->vbo_, vertex_info);
    render(this->program_handle_);
  }

public:
  static red_triangle make()
  {
    red_triangle triangle;

    global_bind_vao(triangle.vao_);
    glEnableVertexAttribArray(RED_TRIANGLE_VPOS);

    // configure OpenGL to associate data with vertex  attribute 0 to be read in the following way
    glVertexAttribPointer(RED_TRIANGLE_VPOS, NUM_VERTICES, TYPE_OF_DATA, NORMALIZE_DATA, STRIDE_SIZE,
                            OFFSET);
    global_unbind_vao(triangle.vao_);
    return triangle;
  }
};

} // opengl
} // ns gfx
} // ns engine
