#pragma once
#include <string>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ecst.hpp>

#include <engine/gfx/shapes.hpp>
#include <engine/window/window.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/data_types.hpp>

namespace game
{
namespace boomhs
{

class boomhs_game
{
  game::world_coordinate wc0_ = game::world_coordinate{-0.5f, 0.0f, 0.0f, 1.0f};
  game::world_coordinate wc1_ = game::world_coordinate{0.5f, 0.0f, 0.0f, 1.0f};

public:
  boomhs_game() = default;

  NO_COPY(boomhs_game);
  MOVE_DEFAULT(boomhs_game);

  template <typename S>
  void game_loop(S &state)
  {
    ::engine::gfx::opengl::render_args<decltype(state.logger)> const args{state.logger,
      state.view, state.projection, this->wc0_, this->wc1_};

    state.renderer.draw(args);
  }
};

} // ns boomhs
} // ns game
