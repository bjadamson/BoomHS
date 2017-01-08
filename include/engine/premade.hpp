#pragma once
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>
#include <string>

#include <opengl/lib.hpp>
#include <window/window.hpp>
#include <window/sdl_window.hpp>

namespace engine
{

using ON_DESTROY = void (*)();

template<typename E>
struct premade
{
  E engine_;
  ON_DESTROY on_destroy_;

  NO_MOVE_ASSIGN(premade);
  NO_COPY(premade);
public:
  explicit premade(E &&e, ON_DESTROY const fn)
    : engine_(MOVE(e))
    , on_destroy_(fn)
  {
  }

  premade(premade &&o)
    : engine_(MOVE(o.engine_))
    , on_destroy_(o.on_destroy_)
  {
    o.on_destroy_ = nullptr;
  }

  ~premade()
  {
    if (nullptr != this->on_destroy_) {
      this->on_destroy_();
    }
  }

  auto &engine() { return this->engine_; }
};

template<typename E>
auto
make_premade(E &&e, ON_DESTROY const fn)
{
  return premade<E>{MOVE(e), fn};
}

template<typename GFX_LIB>
using premade_result = stlw::result<premade<engine<GFX_LIB>>, std::string>;

template<typename L>
premade_result<opengl::opengl_lib>
make_opengl_sdl_premade_configuration(L &logger, float const width, float const height)
{
  // Select windowing library as SDL.
  namespace w = window;
  using window_lib = w::library_wrapper<w::sdl_library>;

  LOG_DEBUG("Initializing window library globals");
  DO_TRY(auto _, window_lib::init());

  LOG_DEBUG("Instantiating window instance.");
  DO_TRY(auto window, window_lib::make_window(height, width));

  DO_TRY(auto opengl, opengl::lib_factory::make(logger));
  auto engine = engine_factory::make_engine(logger, MOVE(window), MOVE(opengl));

  return make_premade(MOVE(engine), &window_lib::destroy);
}

} // ns engine
