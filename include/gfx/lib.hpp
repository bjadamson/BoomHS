#pragma once
#include <gfx/camera.hpp>
#include <gfx/pipeline.hpp>

#include <stlw/burrito.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

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

template<typename LIB, typename P>
class gfx_lib
{
  friend struct lib_factory;
  NO_COPY(gfx_lib);
public:
  LIB draw_lib;
  P pipelines;

private:
  explicit gfx_lib(LIB &&lib, P &&p)
      : draw_lib(MOVE(lib))
      , pipelines(MOVE(p))
  {
  }

  template <typename Args, typename Wrappable>
  void draw_wrappable(Args const& args, Wrappable const& wrappable)
  {
    auto const draw_fn = [&](auto &&pipeline_shape_pair)
    {
      auto &pipeline = pipeline_shape_pair.pipeline;
      auto &&shape = MOVE(pipeline_shape_pair.shape);
      this->draw_lib.draw(args, pipeline.backend(), MOVE(shape));
    };
    auto burrito = stlw::make_burrito(wrappable);
    stlw::hof::for_each(burrito, draw_fn);
  }

  template <typename Args, typename Wrappable>
  void draw_wrappable(Args const& args, Wrappable &&wrappable)
  {
    auto const draw_fn = [&](auto &&pipeline_shape)
    {
      auto &pipeline = pipeline_shape.pipeline;
      auto &&shape = MOVE(pipeline_shape.shape);
      this->draw_lib.draw(args, pipeline.backend(), MOVE(shape));
    };
    auto burrito = stlw::make_burrito(MOVE(wrappable));
    stlw::hof::for_each(MOVE(burrito), draw_fn);
  }

public:
  MOVE_DEFAULT(gfx_lib);

  void begin()
  {
    this->draw_lib.begin();
  }

  template<typename Args, template<class, std::size_t> typename Container, typename T, std::size_t N>
  void
  draw(Args const& args, Container<T, N> const& arr)
  {
    auto x = stlw::tuple_from_array(arr);
    this->draw_wrappable(args, MOVE(x));
  }

  // The last parameter type here ensures that the value passed is similar to a stl container.
  template<typename Args, typename Container, typename IGNORE= typename Container::value_type>
  void
  draw(Args const& args, Container &&c)
  {
    this->draw_wrappable(args, MOVE(c));
  }

  template<typename Args, typename ...T>
  void
  draw(Args const& args, T &&... t)
  {
    auto tuple = std::make_tuple(MOVE(t)...);
    this->draw_wrappable(args, tuple);
  }

  void end()
  {
    this->draw_lib.end();
  }
};

struct lib_factory {
  lib_factory() = delete;
  ~lib_factory() = delete;

  template <typename L, typename BACKEND_LIB>
  static auto make(L &logger, BACKEND_LIB &&backend_lib)
  {
    auto draw_lib = MOVE(backend_lib.draw_lib);
    auto pipelines = MOVE(backend_lib.pipelines);
    return gfx_lib<decltype(draw_lib), decltype(pipelines)>{MOVE(draw_lib), MOVE(pipelines)};
  }
};

} // ns gfx
