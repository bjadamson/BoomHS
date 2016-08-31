#pragma once
#include <string>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/gfx/shapes.hpp>
#include <engine/window/window.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/data_types.hpp>

namespace {

constexpr ::engine::gfx::triangle
wc_to_gfx_triangle(game::world_coordinate const& wc)
{
  using namespace engine::gfx;
  // Set up vertex data (and buffer(s)) and attribute pointers
  constexpr float w_coord = 1.0f;
  constexpr float radius = 0.5;
  // clang-format off
 std::array<float, 12> v0 =
  {
    wc.x - radius, wc.y - radius, 0.0f, w_coord, // bottom left
    wc.x + radius, wc.y - radius, 0.0f, w_coord, // bottom right
    wc.x         , wc.y + radius, 0.0f, w_coord  // top middle
  };
  constexpr std::array<float, 12> c0 =
  {
    1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
  };
  return ::engine::gfx::make_triangle(v0, c0);
  // clang-format on
}

} // ns anon

namespace game
{
namespace boomhs
{

template <typename L>
class boomhs_game
{
  L &logger_;
  engine::window::dimensions const dimensions_;

  game::world_coordinate wc0_;
  game::world_coordinate wc1_;

  glm::mat4 view, projection;

  boomhs_game(L &l, engine::window::dimensions const &d, game::world_coordinate const &w0,
              game::world_coordinate const &w1)
      : logger_(l)
      , dimensions_(d)
      , wc0_(w0)
      , wc1_(w1)
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
    auto const t0 = wc_to_gfx_triangle(this->wc0_);
    auto const t1 = wc_to_gfx_triangle(this->wc1_);

    ::engine::gfx::opengl::render_args<decltype(this->logger_)> const args{this->logger_, view,
      projection, t0, t1};

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
    auto w0 = game::world_coordinate{-0.5f, 0.0f, 0.0f, 1.0f};
    auto w1 = game::world_coordinate{0.5f, 0.0f, 0.0f, 1.0f};

    return boomhs_game<L>{logger, dimensions, w0, w1};
  }
};

} // ns boomhs
} // ns game
