#pragma once
#include <boomhs/inventory.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/tile.hpp>
#include <stlw/log.hpp>

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
  pickup_entity(EntityID, EntityRegistry &);

  // Removes entity from the player
  static void
  drop_entity(stlw::Logger &, EntityID, EntityRegistry &);
};

} // ns boomhs
