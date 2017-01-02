#pragma once
#include <stlw/type_macros.hpp>
#include <gfx/opengl/engine.hpp>
#include <gfx/opengl/pipeline.hpp>

namespace gfx::opengl
{

class opengl_lib
{
  friend struct lib_factory;
public:
  engine gfx_engine;
private:
  pipeline_factory pipeline_factory_;

  opengl_lib(engine &&e, pipeline_factory &&pf)
    : gfx_engine(MOVE(e))
    , pipeline_factory_(MOVE(pf))
    {
    }
public:
  MOVE_CONSTRUCTIBLE_ONLY(opengl_lib);
};

struct lib_factory
{
  lib_factory() = delete;

  template<typename L>
  static stlw::result<opengl_lib, std::string>
  make(L &logger)
  {
    DO_TRY(auto engine, engine_factory::make(logger));
    return opengl_lib{MOVE(engine), pipeline_factory{}};
  }
};

} // ns gfx::opengl
