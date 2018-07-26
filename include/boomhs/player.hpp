#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/inventory.hpp>
#include <stlw/log.hpp>

namespace boomhs
{
struct Item;

struct PlayerData
{
  Inventory inventory;
};

struct Player
{
  Player() = delete;

  static void pickup_entity(EntityID, EntityRegistry&);

  // Removes entity from the player
  static void drop_entity(stlw::Logger&, EntityID, EntityRegistry&);

  static glm::vec3 world_position(EntityID, EntityRegistry&);
};

inline EntityID
find_player(EntityRegistry& registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<PlayerData>().size());

  // Assume Player has a Transform
  auto                    view = registry.view<PlayerData, Transform>();
  std::optional<EntityID> entity{std::nullopt};
  for (auto const e : view) {
    // This assert ensures this loop only runs once.
    assert(std::nullopt == entity);
    entity = e;
  }
  assert(std::nullopt != entity);
  return *entity;
}

} // namespace boomhs
