#pragma once
#include <stlw/burrito.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/camera.hpp>
#include <opengl/pipeline.hpp>
#include <opengl/renderer.hpp>

namespace opengl
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

class opengl_draw_lib
{
  friend struct lib_factory;
  opengl_renderer renderer_;
private:
  explicit opengl_draw_lib(struct opengl_renderer &&r)
    : renderer_(MOVE(r))
    {
    }

  /*
  template<typename ...Args>
  void
  draw_impl(Args &&... args)
  {
    this->renderer_.draw(std::forward<Args>(args)...);
  }

  template <typename Args, typename Wrappable>
  void draw_wrappable(Args const& args, Wrappable const& wrappable)
  {
    auto const draw_fn = [&](auto &&pipeline_shape_pair)
    {
      auto &pipeline = pipeline_shape_pair.pipeline;
      auto &&shape = MOVE(pipeline_shape_pair.shape);
      this->draw_impl(args, pipeline, MOVE(shape));
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
      this->draw_impl(args, pipeline, MOVE(shape));
    };
    auto burrito = stlw::make_burrito(MOVE(wrappable));
    stlw::hof::for_each(MOVE(burrito), draw_fn);
  }
  */

public:
  MOVE_CONSTRUCTIBLE_ONLY(opengl_draw_lib);

  template<typename ...Args>
  void
  draw_impl(Args const&... args)
  {
    this->renderer_.draw(std::forward<Args>(args)...);
  }

  template<typename Args, typename T>
  void
  draw(Args const& args, T &pipeline_shape)
  {
      auto &pipeline = pipeline_shape.pipeline;
      auto const& shape = MOVE(pipeline_shape.shape);
      this->renderer_.draw(args, pipeline, shape);
  }

  //template<typename Args, template<class, std::size_t> typename Container, typename T, std::size_t N>
  //void
  //draw(Args const& args, Container<T, N> const& arr)
  //{
    //auto x = stlw::tuple_from_array(arr);
    //this->draw_wrappable(args, MOVE(x));
  //}

  // The last parameter type here ensures that the value passed is similar to a stl container.
  //template<typename Args, typename Container, typename IGNORE= typename Container::value_type>
  //void
  //draw(Args const& args, Container &&c)
  //{
    //this->draw_wrappable(args, MOVE(c));
  //}

  //template<typename Args, typename ...T>
  //void
  //draw(Args const& args, T &&... t)
  //{
    //auto tuple = std::make_tuple(MOVE(t)...);
    //this->draw_wrappable(args, tuple);
  //}

  void begin()
  {
    this->renderer_.begin();
  }

  void end()
  {
    this->renderer_.end();
  }
};

struct opengl_lib
{
  opengl_draw_lib draw_lib;
  opengl_pipelines pipelines;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_lib);
  opengl_lib(opengl_draw_lib && dlib, opengl_pipelines &&p)
    : draw_lib(MOVE(dlib))
    , pipelines(MOVE(p))
  {
  }
};

struct lib_factory
{
  lib_factory() = delete;

  template<typename L>
  static stlw::result<opengl_lib, std::string>
  make(L &logger)
  {
    auto contexts = opengl_contexts_factory::make(logger);
    DO_TRY(auto pipelines, opengl_pipelines_factory::make(logger, MOVE(contexts)));
    opengl_draw_lib draw_lib{opengl_renderer_factory::make(logger)};
    return opengl_lib{MOVE(draw_lib), MOVE(pipelines)};
  }
};

} // ns opengl
