#pragma once
#include <engine/gfx/opengl/engine.hpp>
#include <engine/window/sdl_window.hpp>
#include <stlw/type_ctors.hpp>

namespace engine::gfx
{

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
  friend struct factory;

  W window_;
  opengl::engine engine_;

  gfx_engine(W &&w, opengl::engine &&e)
    : window_(std::move(w))
    , engine_(std::move(e))
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  }

  NO_COPY(gfx_engine);
public:
  MOVE_DEFAULT(gfx_engine);

  void begin()
  {
    // Render
    this->engine_.begin();
  }

  template <typename Args, typename ...S>
  void draw0(Args const& args, S const&... shapes)
  {
    this->engine_.draw0(args, shapes...);
  }

  template <typename Args, typename ...S>
  void draw1(Args const& args, S const&... shapes)
  {
    this->engine_.draw1(args, shapes...);
  }

  void end()
  {
    this->engine_.end();

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L, typename W>
  static stlw::result<gfx_engine, std::string> make_gfx_sdl_engine(L &logger, W &&window)
  {
    DO_MONAD(auto opengl_engine, opengl::factory::make_opengl_engine(logger));

    // TODO: move this
    //glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
    return gfx_engine{std::move(window), std::move(opengl_engine)};
  }
};

} // ns engine::gfx
