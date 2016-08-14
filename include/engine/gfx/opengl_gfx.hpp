#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl_glew.hpp>
#include <engine/gfx/opengl/red_triangle.hpp>
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

auto const bind_vao = [](auto const vao) {
  glBindVertexArray(vao);
};

auto const unbind_vao = [](auto const vao) {
  glBindVertexArray(0);
};

class renderer
{
  using W = ::engine::window::sdl_window;

  red_triangle red_;
  W window_;

  renderer(W &&w)
      : window_(std::move(w))
      , red_(red_triangle::make())
  {
  }

  NO_COPY(renderer);
  renderer &operator=(renderer &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  renderer(renderer &&other)
      : red_(std::move(other.red_))
      , window_(std::move(other.window_))
  {
  }

  void init_buffers()
  {
    this->red_.init_buffers();
  }

  void destroy_buffers()
  {
    this->red_.destroy_buffers();
  }

  void draw(GLfloat const v0[12], GLfloat const v1[12])
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    this->red_.draw(v0, v1);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
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
      return stlw::make_error(expected_program_id.error());
    }
    this->red_.program_handle_ = std::move(*expected_program_id);
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
