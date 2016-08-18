#pragma once
#include <string>

#include <engine/gfx/shapes.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

namespace game
{
namespace boomhs
{

template <typename L>
class boomhs_game
{
  L &l_;

  boomhs_game(L &l)
      : l_(l)
  {
  }

  friend struct boomhs_library;

  inline static bool is_quit_event(SDL_Event &event)
  {
    bool is_quit = false;

    switch (event.type) {
    case SDL_QUIT: {
      is_quit = true;
      break;
    }
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE: {
        is_quit = true;
        break;
      }
      }
    }
    }
    return is_quit;
  }

public:
  NO_COPY(boomhs_game);
  MOVE_DEFAULT(boomhs_game);

  template <typename R>
  stlw::result<stlw::empty_type, std::string> init(R &renderer)
  {
    // nothing to do for now
    return stlw::make_empty();
  }

  template <typename R>
  void game_loop(R &renderer)
  {
    // Set up vertex data (and buffer(s)) and attribute pointers
    constexpr float Wcoord = 1.0f;
    // clang-format off
    constexpr std::array<float, 12> v0 =
    {
      -0.5f , -0.5f, 0.0f, Wcoord, // bottom left
      0.0f  , -0.5f, 0.0f, Wcoord, // bottom right
      -0.25f, 0.5f , 0.0f, Wcoord  // top middle
    };
    constexpr std::array<float, 12> v1 =
    {
      0.5f  , 0.0f, 0.0f, Wcoord, // bottom left
      1.00f , 0.0f, 0.0f, Wcoord, // bottom right
      0.75f , 1.0f, 0.0f, Wcoord  // top middle
    };
    constexpr std::array<float, 12> c0 =
    {
      1.0f, 0.0f, 0.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
    };
    // clang-format on
    using namespace engine::gfx;
    constexpr triangle t0 = make_triangle(v0, c0);
    constexpr triangle t1 = make_triangle(v1, c0);

    renderer.draw(t0, t1);
  }

  bool process_event(SDL_Event &event) { return is_quit_event(event); }
};

struct boomhs_library {
  boomhs_library() = delete;

  DEFINE_STATIC_WRAPPER_FUNCTION(make_game, boomhs_game<P...>);
};

} // ns boomhs
} // ns game
