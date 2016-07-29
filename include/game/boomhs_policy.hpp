#pragma once
#include <engine/window/window.hpp>
#include <game/boomhs.hpp>

namespace game
{

struct boomhs_policy
{
  boomhs_policy() = delete;
  DEFINE_STATIC_WRAPPER_FUNCTION(game_loop, boomhs::game_loop);

  using game_type = boomhs;
};

} // ns game
