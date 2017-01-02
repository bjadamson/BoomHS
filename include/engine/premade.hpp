#pragma once
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>
#include <string>

#include <window/window.hpp>
#include <window/sdl_window.hpp>

namespace engine
{

using ON_DESTROY = void (*)();

struct premade
{
  engine engine_;
  ON_DESTROY on_destroy_;

  NO_MOVE_ASSIGN(premade);
  NO_COPY(premade);
public:
  explicit premade(engine &&e, ON_DESTROY const fn)
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
  return premade{MOVE(e), fn};
}

template<typename L>
stlw::result<premade, std::string>
make_opengl_sdl_premade_configuration(L &logger, float const width, float const height)
{
  // Select windowing library as SDL.
  namespace w = window;
  using window_lib = w::library_wrapper<w::sdl_library>;

  logger.debug("Initializing window library globals");
  DO_TRY(auto _, window_lib::init());

  logger.debug("Instantiating window instance.");
  DO_TRY(auto window, window_lib::make_window(height, width));

  DO_TRY(auto opengl_lib, gfx::opengl::lib_factory::make(logger));
  auto engine_result = factory::make_engine(logger, MOVE(window), MOVE(opengl_lib));
  DO_TRY(auto engine, MOVE(engine_result));

  return make_premade(MOVE(engine), &window_lib::destroy);
}

} // ns engine
