#pragma once
#include <engine/gfx/opengl/engine.hpp>
#include <engine/window/sdl_window.hpp>
#include <stlw/burrito.hpp>
#include <stlw/type_ctors.hpp>

namespace engine::gfx
{

template <typename L>
struct render_args {
  L &logger;
  camera const& camera;
  glm::mat4 const& projection;

  render_args(L &l, class camera const &c, glm::mat4 const &p)
      : logger(l)
      , camera(c)
      , projection(p)
  {
  }
};

class gfx_engine
{
  using W = ::engine::window::sdl_window;
  friend struct factory;

  W window_;
public:
  opengl::engine engine;
private:

  gfx_engine(W &&w, opengl::engine &&e)
      : window_(std::move(w))
      , engine(std::move(e))
  {
  }

  NO_COPY(gfx_engine);

public:
  MOVE_DEFAULT(gfx_engine);

  void begin()
  {
    this->engine.begin();
  }

  template <typename Args, typename C, typename... S>
  void draw(Args const& args, C &ctx, S const&... shapes)
  {
    this->draw_impl(args, ctx, stlw::make_burrito(std::make_tuple(shapes...)));
  }

  template <typename Args, typename C, typename... S>
  void draw(Args const& args, C &ctx, std::tuple<S...> const& shapes)
  {
    this->draw_impl(args, ctx, stlw::make_burrito(shapes));
  }

  template <typename Args, typename C, typename B>
  void draw_impl(Args const& args, C &ctx, B const& burrito)
  {
    this->engine.draw(args, ctx, burrito.unwrap());
  }

  void end()
  {
    this->engine.end();

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
    DO_TRY(auto opengl_engine, opengl::factory::make_opengl_engine(logger));
    return gfx_engine{std::move(window), std::move(opengl_engine)};
  }
};

} // ns engine::gfx
