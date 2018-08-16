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
  EntityID eid_;
  EntityRegistry* registry_;

  GCD gcd;

public:
  explicit Player(EntityID, EntityRegistry&, glm::vec3 const&, glm::vec3 const&);

  WorldObject  world_object;
  Inventory    inventory;
  HealthPoints hp{44, 50};
  int          level = -1;
  std::string  name;

  bool is_attacking = false;
  int   damage       = 1;
  float speed;

  void pickup_entity(EntityID, EntityRegistry&);
  void drop_entity(common::Logger&, EntityID, EntityRegistry&);

  void try_pickup_nearby_item(common::Logger&, EntityRegistry&, window::FrameTime const&);

  void update(EngineState&, ZoneState&, window::FrameTime const&);

  auto const& transform() const { return registry_->get<Transform>(eid_); }
  auto&       transform() { return registry_->get<Transform>(eid_); }

  auto const& bounding_box() const { return registry_->get<AABoundingBox>(eid_); }

  glm::vec3 world_position() const;
};

EntityID
find_player(EntityRegistry&);

} // namespace boomhs
