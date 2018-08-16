#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/gcd.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/world_object.hpp>
#include <extlibs/glm.hpp>

#include <common/log.hpp>
#include <optional>
#include <string>

namespace opengl
{
class TextureTable;
} // namespace opengl

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
class EngineState;
struct Item;
class NearbyTargets;
class TerrainGrid;
struct ZoneState;

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
  void drop_entity(common::Logger&, EntityID, EntityRegistry&);

  void try_pickup_nearby_item(common::Logger&, EntityRegistry&, window::FrameTime const&);

  void update(EngineState&, ZoneState&, window::FrameTime const&);

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
