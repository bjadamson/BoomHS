#pragma once
#include <string>

#include <engine/gfx/shapes.hpp>
#include <engine/window/window.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace game
{
namespace boomhs
{

template <typename L>
class boomhs_game
{
  L &logger_;
  engine::window::dimensions const dimensions_;

  engine::gfx::triangle t0_;
  engine::gfx::triangle t1_;

  glm::mat4 view, projection;

  boomhs_game(L &l, engine::window::dimensions const &d, engine::gfx::triangle const &t0,
              engine::gfx::triangle const &t1)
      : logger_(l)
      , dimensions_(d)
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
    ::engine::gfx::opengl::render_args<decltype(this->logger_)> const args{this->logger_, view,
      projection, this->t0_, this->t1_};

    renderer.draw(args);
  }

  bool process_event(SDL_Event &event)
  {
    float constexpr DISTANCE = 0.1f;
    float constexpr ANGLE = 0.2f;
    float constexpr SCALE_FACTOR = 0.1f;

    auto const make_scalev = [](float const f) { return glm::vec3(1.0f + f, 1.0f + f, 1.0f + f); };

    switch (event.type) {
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      case SDLK_w: {
        this->view = glm::translate(this->view, glm::vec3(0.0f, DISTANCE, 0.0f));
        break;
      }
      case SDLK_s: {
        this->view = glm::translate(this->view, glm::vec3(0.0f, -DISTANCE, 0.0f));
        break;
      }
      case SDLK_a: {
        this->view = glm::translate(this->view, glm::vec3(-DISTANCE, 0.0f, 0.0f));
        break;
      }
      case SDLK_d: {
        this->view = glm::translate(this->view, glm::vec3(DISTANCE, 0.0f, 0.0f));
        break;
      }
      // scaling
      case SDLK_o: {
        this->view = glm::scale(this->view, make_scalev(SCALE_FACTOR));
        break;
      }
      case SDLK_p: {
        this->view = glm::scale(this->view, make_scalev(-SCALE_FACTOR));
        break;
      }
      // z-rotation
      case SDLK_j: {
        this->view = glm::rotate(this->view, ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      case SDLK_k: {
        this->view = glm::rotate(this->view, -ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      // y-rotation
      case SDLK_u: {
        this->view = glm::rotate(this->view, ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      case SDLK_i: {
        this->view = glm::rotate(this->view, -ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      // x-rotation
      case SDLK_n: {
        this->view = glm::rotate(this->view, ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      case SDLK_m: {
        this->view = glm::rotate(this->view, -ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      }
    }
    }
    return is_quit_event(event);
  }
};

struct boomhs_library {
  boomhs_library() = delete;

  template <typename L>
  static inline auto make_game(L &logger, engine::window::dimensions const &dimensions)
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

    return boomhs_game<L>{logger, dimensions, t0, t1};
  }
};

} // ns boomhs
} // ns game
