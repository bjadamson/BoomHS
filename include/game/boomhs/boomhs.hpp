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
  L &logger_;
  engine::gfx::triangle t0_;
  engine::gfx::triangle t1_;

  boomhs_game(L &l, engine::gfx::triangle const& t0, engine::gfx::triangle const& t1)
      : logger_(l)
      , t0_(t0)
      , t1_(t1)
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
    renderer.draw(this->logger_, this->t0_, this->t1_);
  }

  bool process_event(SDL_Event &event) { return is_quit_event(event); }
};

struct boomhs_library {
  boomhs_library() = delete;

  template<typename L>
  static inline auto
  make_game(L &logger)
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
    triangle t0 = make_triangle(v0, c0);
    triangle t1 = make_triangle(v1, c0);

    return boomhs_game<L>{logger, t0, t1};
  }
};

} // ns boomhs
} // ns game
