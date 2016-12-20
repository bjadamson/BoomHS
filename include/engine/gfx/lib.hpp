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

using namespace stlw;

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

  template <typename Args, typename Ctx, typename B>
  void draw_burrito(Args const& args, Ctx &ctx, B const& burrito)
  {
    this->engine.draw(args, ctx, stlw::make_burrito(burrito));
  }

  template <typename Args, typename Ctx, typename B>
  void draw_burrito(Args const& args, Ctx &ctx, B &&burrito)
  {
    this->engine.draw(args, ctx, stlw::make_burrito(std::move(burrito)));
  }

public:
  MOVE_DEFAULT(gfx_engine);

  void begin()
  {
    this->engine.begin();
  }

  template<typename Args, typename Ctx, template<class, std::size_t> typename C, typename T, std::size_t N>
  void
  draw(Args const& args, Ctx &ctx, C<T, N> const& arr)
  {
    auto x = stlw::tuple_from_array(arr);
    this->draw_burrito(args, ctx, std::move(x));
  }

  template<typename Args, typename Ctx, template<class, std::size_t> typename C, typename T, std::size_t N>
  void
  draw(Args const& args, Ctx &ctx, C<T, N> &&arr)
  {
    auto const a = std::move(arr);
    this->draw(args, ctx, a);
  }

  /*
  template<typename Args, typename Ctx, typename C>
  void
  draw(Args const& args, Ctx &ctx, C const& c)
  {
    std::array<C, 1> const arr{c};
    auto x = stlw::tuple_from_array(arr);
    this->draw_burrito(args, ctx, std::move(x));
  }
  */

  // The last parameter type here ensures that the value passed is similar to a stl container.
  template<typename Args, typename Ctx, typename C, typename IGNORE= typename C::value_type>
  void
  draw(Args const& args, Ctx &ctx, C &&c)
  {
    this->draw_burrito(args, ctx, std::move(c));
  }

  template<typename Args, typename Ctx, typename ...T>
  void
  draw(Args const& args, Ctx &ctx, std::tuple<T...> &&t)
  {
    using U = std::tuple<T...>;
    this->draw_burrito(args, ctx, std::move(t));
  }

  template<typename Args, typename Ctx, typename ...T>
  void
  draw(Args const& args, Ctx &ctx, T const&... t)
  {
    this->draw_burrito(args, ctx, std::make_tuple(t...));
  }

  template<typename Args, typename Ctx, typename ...T>
  void
  draw(Args const& args, Ctx &ctx, T &&... t)
  {
    this->draw_burrito(args, ctx, std::make_tuple(std::forward<T>(t)...));
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
