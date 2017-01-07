#pragma once
#include <stlw/type_macros.hpp>
#include <opengl/pipeline.hpp>
#include <opengl/renderer.hpp>

namespace opengl
{

class opengl_draw_lib
{
  friend struct lib_factory;
  opengl_renderer renderer_;
private:
  opengl_draw_lib(struct opengl_renderer &&r)
    : renderer_(MOVE(r))
    {
    }
public:
  MOVE_CONSTRUCTIBLE_ONLY(opengl_draw_lib);

  template<typename ...Args>
  void
  draw(Args &&... args)
  {
    this->renderer_.draw(std::forward<Args>(args)...);
  }

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