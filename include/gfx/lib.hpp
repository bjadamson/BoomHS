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

  template <typename Args, typename Wrappable>
  void draw_wrappable(Args const& args, Wrappable const& wrappable)
  {
    auto const draw_fn = [&](auto &&both)
    {
      auto &pipeline = both.pipeline;
      auto shape = MOVE(both.shape);
      this->lib.draw(args, pipeline.backend(), shape);
    };
    auto burrito = stlw::make_burrito(wrappable);
    stlw::hof::for_each(burrito, draw_fn);
  }

  template <typename Args, typename Wrappable>
  void draw_wrappable(Args const& args, Wrappable &&wrappable)
  {
    auto const draw_fn = [&](auto &&both)
    {
      auto &pipeline = both.pipeline;
      auto shape = MOVE(both.shape);
      this->lib.draw(args, pipeline.backend(), MOVE(shape));
    };
    auto burrito = stlw::make_burrito(MOVE(wrappable));
    stlw::hof::for_each(burrito, draw_fn);
  }

public:
  MOVE_DEFAULT(gfx_lib);

  void begin()
  {
    this->lib.begin();
  }

  // TODO: poc
  auto
  make_shape_factories()
  {
    return impl::make_shape_factories(this->lib.opengl_pipelines);
  }

  /*
  template<typename Args, template<class, std::size_t> typename Container, typename T, std::size_t N>
  void
  draw(Args const& args, Container<T, N> const& arr)
  {
    auto x = stlw::tuple_from_array(arr);
    this->draw_wrappable(args, pipeline, MOVE(x));
  }

  // The last parameter type here ensures that the value passed is similar to a stl container.
  template<typename Args, typename Container, typename IGNORE= typename Container::value_type>
  void
  draw(Args const& args, Container &&c)
  {
    this->draw_wrappable(args, pipeline, MOVE(c));
  }
  */

  template<typename Args, typename P, typename ...T>
  void
  draw_impl(Args const& args, P &pipeline, std::tuple<T...> &&t)
  {
    this->draw_wrappable(args, pipeline, MOVE(t));
  }

  /*
  template<typename Args, typename ...T>
  void
  draw(Args const& args, P &pipeline, T &&... t)
  {
    this->draw_wrappable(args, pipeline, std::make_tuple(std::forward<T>(t)...));
  }
  */

  template<typename Args, typename ...T>
  void
  draw(Args const& args, T &&... t)
  {
    auto tuple = std::make_tuple(t...);
    this->draw_wrappable(args, tuple);
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
