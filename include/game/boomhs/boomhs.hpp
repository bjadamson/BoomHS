#pragma once
#include <string>

#include <stlw/result.hpp>
#include <game/game.hpp>

namespace game
{
namespace boomhs
{

struct boomhs_game
{
  using window_type = ::engine::window::window;

  boomhs_game() = delete;

  static stlw::result<stlw::empty_type, std::string>
  game_loop(window_type &&);
};

struct policy
{
  policy() = delete;
  using game_type = boomhs_game;

  DEFINE_STATIC_WRAPPER_FUNCTION(game_loop, game_type::game_loop);
};

} // ns boomhs
} // ns game
