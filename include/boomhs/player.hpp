#pragma once
#include <boomhs/tile.hpp>

namespace boomhs
{

struct Player
{
  TilePosition tile_position;
  bool inventory_open = false;
};

} // ns boomhs
