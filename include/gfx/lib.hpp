#pragma once
#include <gfx/opengl/lib.hpp>
#include <stlw/burrito.hpp>
#include <stlw/type_ctors.hpp>

namespace gfx
{

template <typename L>
struct render_args {
  L &logger;
  camera const& camera;
  glm::mat4 const& projection;
};

template<typename L>
auto make_render_args(L &l, camera const& c, glm::mat4 const& projection)
{
  return render_args<L>{l, c, projection};
}

using namespace stlw;

class gfx_lib
{
  friend struct factory;
  NO_COPY(gfx_lib);
public:
  opengl::opengl_lib lib;
private:
  gfx_lib(opengl::opengl_lib &&l)
      : lib(MOVE(l))
  {
  }

  template <typename Args, typename Ctx, typename B>
  void draw_burrito(Args const& args, Ctx &ctx, B const& burrito)
  {
    this->lib.gfx_engine.draw(args, ctx, stlw::make_burrito(burrito));
  }

  template <typename Args, typename Ctx, typename B>
  void draw_burrito(Args const& args, Ctx &ctx, B &&burrito)
  {
    this->lib.gfx_engine.draw(args, ctx, stlw::make_burrito(std::move(burrito)));
  }

public:
  MOVE_DEFAULT(gfx_lib);

  void begin()
  {
    this->lib.gfx_engine.begin();
  }

  template<typename Args, typename Ctx, template<class, std::size_t> typename C, typename T, std::size_t N>
  void
  draw(Args const& args, Ctx &ctx, C<T, N> const& arr)
  {
    auto x = stlw::tuple_from_array(arr);
    this->draw_burrito(args, ctx, std::move(x));
  }

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
    this->draw_burrito(args, ctx, std::move(t));
  }

  template<typename Args, typename S>
  void
  draw_special(Args const& args, S &&s)
  {
    this->draw(args, s.context, std::move(s.data));
  }

  template<typename Args, typename Ctx, typename ...T>
  void
  draw(Args const& args, Ctx &ctx, T &&... t)
  {
    this->draw_burrito(args, ctx, std::make_tuple(std::forward<T>(t)...));
  }

  void end()
  {
    this->lib.gfx_engine.end();
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L>
  static stlw::result<gfx_lib, std::string> make_gfx_engine(L &logger)
  {
    DO_TRY(auto opengl_lib, opengl::lib_factory::make(logger));
    return gfx_lib{MOVE(opengl_lib)};
  }
};

} // ns gfx
