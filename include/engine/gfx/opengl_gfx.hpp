#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl_glew.hpp>
#include <engine/window/sdl_window.hpp>
#include <stlw/type_ctors.hpp>

namespace engine
{
namespace gfx
{
namespace opengl
{

struct vertex_shader_filename {
  char const *filename;
  vertex_shader_filename(char const *f)
      : filename(f)
  {
  }
};
struct fragment_shader_filename {
  char const *filename;
  fragment_shader_filename(char const *f)
      : filename(f)
  {
  }
};

auto log_error = [](auto const line) {
  GLenum err = GL_NO_ERROR;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "GL error! '" << std::hex << err << "' line #" << std::to_string(line)
              << std::endl;
    return;
  }
  std::string error = SDL_GetError();
  if (error != "") {
    std::cout << "SLD Error : " << error << std::endl;
    SDL_ClearError();
  }
};

auto const print_matrix = [](auto const &matrix) {
  for (auto i = 0; i < 16; ++i) {
    if ((i > 0) && ((i % 4) == 0)) {
      std::cerr << "\n";
    }
    std::cerr << " "
              << "[" << std::to_string(matrix[i]) << "]";
  }
  std::cerr << "\n" << std::endl;
};

class renderer
{
  using W = ::engine::window::sdl_window;

  GLuint VAO, VBO;
  program_handle program_handle_;
  W window_;

  renderer(W &&w)
      : window_(std::move(w))
      , VAO(0)
      , VBO(0)
      , program_handle_(program_handle::make_invalid())
  {
  }

  NO_COPY(renderer);
  renderer &operator=(renderer &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  renderer(renderer &&other)
      : VBO(other.VBO)
      , VAO(other.VAO)
      , program_handle_(std::move(other.program_handle_))
      , window_(std::move(other.window_))
  {
    other.VBO = 0;
    other.VAO = 0;
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
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);

    glBindVertexArray(this->VAO);
    glEnableVertexAttribArray(0);
  }

  void destroy_buffers()
  {
    glDeleteBuffers(1, &this->VBO);
    glDeleteVertexArrays(1, &this->VAO);
  }

  void render(program_handle const &program_handle, GLuint const VAO)
  {
    GLuint const program_id = program_handle.get();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // Render
    log_error(__LINE__);
    glClear(GL_COLOR_BUFFER_BIT);
    log_error(__LINE__);

    uint32_t ticks = SDL_GetTicks(), lastticks = 0;
    ticks = SDL_GetTicks();
    if (((ticks * 10 - lastticks * 10)) < 167) {
      SDL_Delay((167 - ((ticks * 10 - lastticks * 10))) / 10);
      log_error(__LINE__);
    }
    lastticks = SDL_GetTicks();
    log_error(__LINE__);

    /////////////////////////////////////////////////////////////////////////////////

    // Draw our first triangle
    glUseProgram(program_id);
    log_error(__LINE__);

    char buffer[2096];
    int actual_length = 0;
    glGetProgramInfoLog(program_id, 2096, &actual_length, buffer);
    if (0 < actual_length) {
      std::cerr << "log: '" << std::to_string(buffer[0]) << "'\n";
    }

    log_error(__LINE__);
    glBindVertexArray(VAO);
    log_error(__LINE__);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    log_error(__LINE__);
    glBindVertexArray(0);
    log_error(__LINE__);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
    log_error(__LINE__);
    SDL_Delay(20);
    log_error(__LINE__);
  }

  void draw(GLfloat const the_vertices[12])
  {
    // print_matrix(vertices);
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

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0); // Unbind when we are done with this scope automatically.
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
    send_vertices_gpu(this->VBO, vertex_info);
    render(this->program_handle_, this->VAO);
  }

  // TODO: Think about how it makes sense to update this state, but more
  // functionally.
  // Maybe we can return a new instance of ourselves instead, with the updated
  // state.
  // IDK.
  stlw::result<stlw::empty_type, std::string>
  load_program(vertex_shader_filename const v, fragment_shader_filename const f)
  {
    auto expected_program_id = program_loader::load(v.filename, f.filename);
    if (!expected_program_id) {
      std::cerr << "shit" << std::endl;
      return stlw::make_error(expected_program_id.error());
    }
    this->program_handle_ = std::move(*expected_program_id);
    return stlw::make_empty();
  }
};

struct opengl_library {
  opengl_library() = delete;

  static void init()
  {
    // for now, to simplify rendering
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
  }

  static void destroy() {}

  DEFINE_STATIC_WRAPPER_FUNCTION(make_renderer, renderer);
};

} // ns opengl
} // ns gfx
} // ns engine
