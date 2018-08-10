#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/gcd.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/world_object.hpp>
#include <extlibs/glm.hpp>

#include <optional>
#include <common/log.hpp>
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

struct PlayerMovement
{
  glm::vec3 forward, backward, left, right;

  glm::vec3 mouse_forward;
};

class Player
{
  GCD gcd;

public:
  Inventory    inventory;
  HealthPoints hp{44, 50};
  int          level = -1;
  std::string  name;

  // Current movement vector
  PlayerMovement movement;

  bool is_attacking = false;
  int  damage       = 1;

  void pickup_entity(EntityID, EntityRegistry&);
  void drop_entity(common::Logger&, EntityID, EntityRegistry&);

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
