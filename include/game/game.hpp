#pragma once
#include <engine/window/sdl_window.hpp> // SDL_Event
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

namespace game
{

// Genric template we expect a library to provide an implementation of.
template <typename L>
struct library_wrapper {
  library_wrapper() = delete;

  template <typename... P>
  inline static decltype(auto) make_game(P &&... p)
  {
    return L::make_game(std::forward<P>(p)...);
  }
};

template <typename Logger, typename WindowDimensions>
class game_executor
{
  Logger &l_;
  WindowDimensions const window_dimensions_;

  NO_COPY(game_executor);

  template <typename Game>
  inline bool loop_once(Game &game)
  {
    SDL_Event event;
    bool quit = false;
    while (0 != SDL_PollEvent(&event)) {
      quit = game.process_event(event);
    }
    return quit;
  }

  friend class game_factory;

public:
  game_executor(Logger &l, engine::window::dimensions const &d)
      : l_(l)
      , window_dimensions_(d)
  {
  }
  MOVE_CONSTRUCTIBLE(game_executor);

  template <typename R, typename GameLib>
  stlw::result<stlw::empty_type, std::string> run(R &&renderer)
  {
    auto instance = GameLib::make_game(this->l_, this->window_dimensions_);
    DO_MONAD(auto _, instance.init(renderer));

    bool quit = false;
    while (!quit) {
      quit = loop_once(instance);
      instance.game_loop(renderer);
    }
    return stlw::make_empty();
  }
};

struct game_factory {
  game_factory() = delete;

  DEFINE_STATIC_WRAPPER_FUNCTION(make_game, game_executor<P...>);
};

} // ns game
