#pragma once
#include <engine/gfx/lib.hpp>
#include <engine/window/sdl_window.hpp>
#include <stlw/type_ctors.hpp>

namespace engine
{

class gfx_lib
{
  using W = ::engine::window::sdl_window;
  friend struct factory;

  W window_;
public:
  gfx::gfx_lib gfx;
  //opengl::engine gfx;
private:
  gfx_lib(W &&w, gfx::gfx_lib &&g)
      : window_(std::move(w))
      , gfx(std::move(g))
  {
  }
  NO_COPY(gfx_lib);
public:
  MOVE_DEFAULT(gfx_lib);

  void begin() { this->gfx.begin(); }

  void end()
  {
    this->gfx.end();

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L, typename W>
  static stlw::result<gfx_lib, std::string> make_gfx_sdl_engine(L &logger, W &&window)
  {
    DO_TRY(auto gfx, gfx::factory::make_gfx_engine(logger));
    return gfx_lib{std::move(window), std::move(gfx)};
  }
};

} // ns engine
