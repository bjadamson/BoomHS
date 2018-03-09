#pragma once
#include <boomhs/item.hpp>
#include <boomhs/tile.hpp>
#include <array>

namespace boomhs
{

struct Inventory
{
  std::array<Item, 40> items;
};

struct PlayerData
{
  TilePosition tile_position;
  bool inventory_open = false;
};

} // ns boomhs
