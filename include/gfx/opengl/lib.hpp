#pragma once
#include <stlw/type_macros.hpp>
#include <gfx/opengl/pipeline.hpp>
#include <gfx/opengl/renderer.hpp>

namespace gfx::opengl
{

class opengl_lib
{
  friend struct lib_factory;
  opengl_renderer renderer_;
public:
  opengl_pipelines opengl_pipelines;
private:
  opengl_lib(struct opengl_renderer &&r, struct opengl_pipelines &&p)
    : renderer_(MOVE(r))
    , opengl_pipelines(MOVE(p))
    {
    }
public:
  MOVE_CONSTRUCTIBLE_ONLY(opengl_lib);

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

struct lib_factory
{
  lib_factory() = delete;

  template<typename L>
  static stlw::result<opengl_lib, std::string>
  make(L &logger)
  {
    auto contexts = opengl_contexts_factory::make(logger);
    DO_TRY(auto pipelines, opengl_pipelines_factory::make(logger, MOVE(contexts)));
    return opengl_lib{opengl_renderer_factory::make(logger), MOVE(pipelines)};
  }
};

} // ns gfx::opengl
