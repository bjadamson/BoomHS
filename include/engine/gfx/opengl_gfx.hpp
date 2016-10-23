#pragma once
#include <engine/gfx/opengl/factory.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/shape_map.hpp>
#include <engine/gfx/opengl_glew.hpp>
#include <engine/gfx/shapes.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/data_types.hpp>
#include <stlw/type_ctors.hpp>
#include <glm/glm.hpp>

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

template<typename L>
struct render_args
{
  L &logger;
  glm::mat4 const& view;
  glm::mat4 const& projection;

  render_args(L &l, glm::mat4 const& v, glm::mat4 const& p)
    : logger(l)
    , view(v)
    , projection(p)
  {
  }
};

class gfx_engine
{
  using W = ::engine::window::sdl_window;

  renderer barrel_texture_;
  W window_;

  gfx_engine(W &&w, renderer &&red)
      : window_(std::move(w))
      , barrel_texture_(std::move(red))
  {
  }

  NO_COPY(gfx_engine);
  gfx_engine &operator=(gfx_engine &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  gfx_engine(gfx_engine &&other)
      : barrel_texture_(std::move(other.barrel_texture_))
      , window_(std::move(other.window_))
  {
  }

  template <typename L, typename ...S>
  void draw(render_args<L> const& args, S const&... shapes)
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    auto const gl_mapped_shapes = map_to_gl(shapes...);

    // Render
    glClear(GL_COLOR_BUFFER_BIT);
    this->barrel_texture_.draw(args.logger, args.view, args.projection, gl_mapped_shapes);

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }
};

struct opengl_library {
  opengl_library() = delete;

  static void init()
  {
    // for now, to simplify rendering
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
  }

  static inline void destroy() {}

  template <typename L, typename W>
  static inline stlw::result<gfx_engine, std::string> make_gfx_engine(L &logger, W &&window)
  {
    DO_MONAD(auto red, factory::make_renderer(logger));
    return gfx_engine{std::move(window), std::move(red)};
  }
};

} // ns opengl
} // ns gfx
} // ns engine
