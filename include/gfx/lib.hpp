#pragma once
// TODO: ditch this
#include <gfx/opengl/lib.hpp>
#include <gfx/pipeline.hpp>
#include <gfx/factory.hpp>

#include <stlw/burrito.hpp>
#include <stlw/type_ctors.hpp>

namespace gfx
{

namespace impl
{

template<typename P>
auto
make_shape_factories(P &pipelines)
{
  auto &d2 = pipelines.d2;
  auto &d3 = pipelines.d3;

  gfx::pipeline_factory pf;
  return gfx::make_shape_factories(
    pf.make_pipeline(d2.color),
    pf.make_pipeline(d2.texture_wall),
    pf.make_pipeline(d2.texture_container),
    pf.make_pipeline(d2.wireframe),

    pf.make_pipeline(d3.color),
    pf.make_pipeline(d3.texture),
    pf.make_pipeline(d3.skybox),
    pf.make_pipeline(d3.wireframe));
}

} // ns impl

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

template<typename B>
class gfx_lib
{
  friend struct factory;
  NO_COPY(gfx_lib);
public:
  B lib;
private:
  gfx_lib(B &&l)
      : lib(MOVE(l))
  {
  }

  template <typename Args, typename Ctx, typename Burrito>
  void draw_burrito(Args const& args, Ctx &ctx, Burrito const& burrito)
  {
    this->lib.draw(args, ctx, stlw::make_burrito(burrito));
  }

  template <typename Args, typename Ctx, typename Burrito>
  void draw_burrito(Args const& args, Ctx &ctx, Burrito &&burrito)
  {
    this->lib.draw(args, ctx, stlw::make_burrito(MOVE(burrito)));
  }

public:
  MOVE_DEFAULT(gfx_lib);

  void begin()
  {
    this->lib.begin();
  }

  // TODO: poc
  auto
  make_shape_factories() const
  {
    return impl::make_shape_factories(this->lib.opengl_pipelines);
  }

  template<typename Args, typename Ctx, template<class, std::size_t> typename C, typename T, std::size_t N>
  void
  draw(Args const& args, Ctx &ctx, C<T, N> const& arr)
  {
    auto x = stlw::tuple_from_array(arr);
    this->draw_burrito(args, ctx, MOVE(x));
  }

  // The last parameter type here ensures that the value passed is similar to a stl container.
  template<typename Args, typename Ctx, typename C, typename IGNORE= typename C::value_type>
  void
  draw(Args const& args, Ctx &ctx, C &&c)
  {
    this->draw_burrito(args, ctx, MOVE(c));
  }

  template<typename Args, typename Ctx, typename ...T>
  void
  draw(Args const& args, Ctx &ctx, std::tuple<T...> &&t)
  {
    this->draw_burrito(args, ctx, MOVE(t));
  }

  template<typename Args, typename Ctx, typename ...T>
  void
  draw(Args const& args, Ctx &ctx, T &&... t)
  {
    this->draw_burrito(args, ctx, std::make_tuple(std::forward<T>(t)...));
  }

  void end()
  {
    this->lib.end();
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L, typename LIB>
  static stlw::result<gfx_lib<LIB>, std::string> make(L &logger, LIB &&gfx)
  {
    return gfx_lib<LIB>{MOVE(gfx)};
  }
};

} // ns gfx
