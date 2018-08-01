#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/item.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

void
kill_entity(stlw::Logger& logger, EntityRegistry& registry, EntityID const eid)
{
  auto const& bbox = registry.get<AABoundingBox>(eid);
  auto const hw = bbox.half_widths();

  // Move the entity half it's bounding box, so it is directly on the ground, or whatever it was
  // standing on.
  auto& transform = registry.get<Transform>(eid);
  transform.translation.y -= hw.y;

  // TODO: Get the normal vector for the terrain at the (x, z) point and rotate the npc to look
  // properly aligned on slanted terrain.
  transform.rotate_degrees(90.0f, opengl::X_UNIT_VECTOR);
}

void
try_attack_selected_target(stlw::Logger& logger, EntityRegistry& registry,
                           EntityID const target_eid)
{
  auto const player_eid = find_player(registry);
  auto&      player      = registry.get<Player>(player_eid);
  auto& ptransform      = registry.get<Transform>(player_eid);
  auto const playerpos  = ptransform.translation;

  auto& npc_transform = registry.get<Transform>(target_eid);
  auto const npcpos   = npc_transform.translation;

  auto& npcdata = registry.get<NPCData>(target_eid);
  auto& target_hp = npcdata.health;
  if (NPC::is_dead(target_hp)) {
    return;
  }

  bool const within_attack_range = glm::distance(npcpos, playerpos) < 2;
  auto& gcd = player.gcd;
  if (within_attack_range) {
    gcd.unpause();
    target_hp.current -= player.damage;

    if (NPC::is_dead(target_hp)) {
      kill_entity(logger, registry, target_eid);
      player.is_attacking = false;
    }
  }
  else {
    gcd.pause();
  }
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Player
void
Player::try_attack_entity(stlw::Logger& logger, EntityID const eid, EntityRegistry& registry)
{
  // If the player is NOT attacking
  // AND
  // if the player's GCD IS ready (meaning player hasn't attacked in at-least the amount of time of
  // time it takes for the GCD to reset)
  //
  // THEN
  // attack the target and reset the GCD
  if (!is_attacking && gcd.is_ready()) {
    try_attack_selected_target(logger, registry, eid);
    gcd.reset();
  }
}

void
Player::update(stlw::Logger& logger, EntityRegistry& registry,
               NearbyTargets& nbt)
{
  gcd.update();
  if (gcd.is_ready() && is_attacking) {
    auto const selected_opt = nbt.selected();
    try_attack_selected_target(logger, registry, *selected_opt);
    gcd.reset();
  }
}

void
Player::pickup_entity(EntityID const eid, EntityRegistry& registry)
{
  auto const player_eid = find_player(registry);
  auto&      player     = registry.get<Player>(player_eid);
  auto&      inventory  = player.inventory;

  assert(inventory.add_item(eid));
  auto& item       = registry.get<Item>(eid);
  item.is_pickedup = true;

  auto& visible = registry.get<IsVisible>(eid);
  visible.value = false;
}

void
Player::drop_entity(stlw::Logger& logger, EntityID const eid, EntityRegistry& registry)
{
  auto const player_eid = find_player(registry);
  auto&      player     = registry.get<Player>(player_eid);
  auto&      inventory  = player.inventory;

  auto& item       = registry.get<Item>(eid);
  item.is_pickedup = false;

  auto& visible = registry.get<IsVisible>(eid);
  visible.value = true;

  // Move the dropped item to the player's position
  auto const& player_pos = registry.get<Transform>(player_eid).translation;

  auto& transform       = registry.get<Transform>(eid);
  transform.translation = player_pos;

  if (registry.has<Torch>(eid)) {
    auto& pointlight = registry.get<PointLight>(eid);
    pointlight.attenuation *= 3.0f;
    LOG_INFO("You have droppped a torch.");
  }
}

glm::vec3
Player::world_position() const
{
  auto& tr = world_object.registry().get<Transform>(world_object.eid());
  return tr.translation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
EntityID
find_player(EntityRegistry& registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Player>().size());

  // Assume Player has a Transform
  auto                    view = registry.view<Player, Transform>();
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
