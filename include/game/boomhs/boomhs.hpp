#pragma once
#include <string>

#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

namespace game
{
namespace boomhs
{

template<typename L>
class boomhs_game
{
  L &l_;

  boomhs_game(L &l) : l_(l) {}

  friend struct boomhs_library;

  inline static bool
  is_quit_event(SDL_Event &event)
  {
    bool is_quit = false;

    switch(event.type)
    {
      case SDL_QUIT:
        {
          is_quit = true;
          break;
        }
      case SDL_KEYDOWN:
        {
          switch(event.key.keysym.sym)
          {
            case SDLK_ESCAPE:
              {
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

  template<typename R>
  stlw::result<stlw::empty_type, std::string>
  init(R &renderer)
  {
    DO_MONAD(auto _, renderer.load_program("shader.vert", "shader.frag"));
    return stlw::make_empty();
  }

  template<typename R>
  void
  game_loop(R &renderer)
  {
    // Set up vertex data (and buffer(s)) and attribute pointers
    constexpr GLfloat Wcoord = 1.0f;
    constexpr GLfloat vertices[12] = {
      -0.5f, -0.5f, 0.0f, Wcoord,
      0.5f, -0.5f, 0.0f, Wcoord,
      0.0f,  0.5f, 0.0f, Wcoord
    };
    renderer.draw(vertices);
  }

  bool
  process_event(SDL_Event &event)
  {
    return is_quit_event(event);
  }
};

struct boomhs_library
{
  boomhs_library() = delete;

  DEFINE_STATIC_WRAPPER_FUNCTION(make_game, boomhs_game<P...>);
};

} // ns boomhs
} // ns game
