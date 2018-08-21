#pragma once
#include <boomhs/bounding_object.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/gcd.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/world_object.hpp>

#include <common/log.hpp>
#include <extlibs/glm.hpp>

#include <common/log.hpp>
#include <optional>
#include <string>

namespace opengl
{
class ShaderPrograms;
class TextureTable;
} // namespace opengl

namespace boomhs
{
class EngineState;
class FrameTime;
struct ZoneState;

class PlayerHead
{
  EntityRegistry* registry_;
  EntityID eid_;

  friend class Player;
public:
  explicit PlayerHead(EntityRegistry&, EntityID, glm::vec3 const&, glm::vec3 const&);
  void update(FrameTime const&);

  // fields
  WorldObject world_object;

  static PlayerHead create(common::Logger&, EntityRegistry&, opengl::ShaderPrograms&);
};

class Player
{
  EntityID        eid_;
  EntityRegistry* registry_;

  WorldObject wo_;
  GCD         gcd_;
  PlayerHead  head_;

public:
  NO_COPY(Player);
  MOVE_DEFAULT(Player);
  explicit Player(common::Logger&, EntityID, EntityRegistry&, opengl::ShaderPrograms&,
                  glm::vec3 const&, glm::vec3 const&);

  Inventory    inventory;
  HealthPoints hp{44, 50};
  int          level = -1;
  std::string  name;

  bool  is_attacking = false;
  int   damage       = 1;
  float speed;

  void pickup_entity(EntityID, EntityRegistry&);
  void drop_entity(common::Logger&, EntityID, EntityRegistry&);

  void try_pickup_nearby_item(common::Logger&, EntityRegistry&, FrameTime const&);

  void update(EngineState&, ZoneState&, FrameTime const&);

  auto const& transform() const { return registry_->get<Transform>(eid_); }
  Transform&  transform() { return registry_->get<Transform>(eid_); }

  auto const& bounding_box() const { return registry_->get<AABoundingBox>(eid_); }

  glm::vec3 world_position() const;

  WorldObject&       head_world_object() { return head_.world_object; }
  WorldObject const& head_world_object() const { return head_.world_object; }

  WorldObject&       world_object() { return wo_; }
  WorldObject const& world_object() const { return wo_; }
};

EntityID
find_player_eid(EntityRegistry&);

Player&
find_player(EntityRegistry&);

} // namespace boomhs
