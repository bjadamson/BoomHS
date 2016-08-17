#pragma once
#include <engine/gfx/opengl/factory.hpp>
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

auto const bind_vao = [](auto const vao) { glBindVertexArray(vao); };
auto const unbind_vao = [](auto const vao) { glBindVertexArray(0); };

class renderer
{
  using W = ::engine::window::sdl_window;

  red_triangle red_;
  W window_;

  renderer(W &&w, red_triangle &&red)
      : window_(std::move(w))
      , red_(std::move(red))
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

  void draw(GLfloat const v0[12], GLfloat const v1[12])
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    this->red_.draw(v0, v1);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
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

  static inline void destroy() {}

  template <typename W>
  static inline stlw::result<renderer, std::string> make_renderer(W &&window)
  {
    DO_MONAD(auto red, factory::make_red_triangle_program());
    return renderer{std::move(window), std::move(red)};
  }
};

} // ns opengl
} // ns gfx
} // ns engine
