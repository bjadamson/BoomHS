#pragma once
#include <string>

#include <stlw/result.hpp>
#include <engine/window/window.hpp>

namespace game
{

struct boomhs
{
  using window_type = ::engine::window::window;

  boomhs() = delete;

  static stlw::result<stlw::empty_type, std::string>
  game_loop(window_type &&);
};

} // ns game
