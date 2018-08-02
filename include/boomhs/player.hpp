#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/gcd.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/world_object.hpp>

#include <optional>
#include <stlw/log.hpp>
#include <string>

namespace boomhs
{
struct Item;
class NearbyTargets;
class TerrainGrid;

class Player
{
  GCD gcd;

public:
  Inventory    inventory;
  HealthPoints hp{44, 50};
  int          level = -1;
  std::string  name;

  bool is_attacking = false;
  int  damage       = 1;

  void pickup_entity(EntityID, EntityRegistry&);
  void drop_entity(stlw::Logger&, EntityID, EntityRegistry&);

  void update(stlw::Logger&, EntityRegistry&, TerrainGrid&, NearbyTargets&);

  glm::vec3 world_position() const;

  // WORLD OBJECT
  //
  // Ensure fields are set!
  WorldObject world_object;

  auto const& transform() const { return world_object.transform(); }
  auto&       transform() { return world_object.transform(); }

  auto const& bounding_box() const { return world_object.bounding_box(); }
};

EntityID
find_player(EntityRegistry&);

} // namespace boomhs
