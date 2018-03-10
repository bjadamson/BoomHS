#pragma once
#include <boomhs/inventory.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/tile.hpp>

namespace boomhs
{
struct Item;
class Inventory;

struct PlayerData
{
  TilePosition tile_position;

  Inventory inventory;
};

struct Player
{
  Player() = delete;

  static void
  add_item(EntityID, Item &, EntityRegistry &);

  static void
  remove_item(EntityID, Item &, EntityRegistry &);

  static void
  remove_item(size_t, Item &, EntityRegistry &);
};

} // ns boomhs
