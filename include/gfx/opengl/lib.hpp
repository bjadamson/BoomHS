#pragma once
#include <stlw/type_macros.hpp>
#include <gfx/opengl/engine.hpp>

namespace gfx::opengl
{

class opengl_lib
{
  friend struct lib_factory;
public:
  opengl_engine opengl_engine;
private:
  opengl_lib(struct opengl_engine &&e)
    : opengl_engine(MOVE(e))
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
    DO_TRY(auto engine, opengl_engine_factory::make(logger));
    return opengl_lib{MOVE(engine)};
  }
};

} // ns gfx::opengl
