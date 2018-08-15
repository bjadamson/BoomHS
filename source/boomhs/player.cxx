#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/item.hpp>
#include <boomhs/item_factory.hpp>
#include <boomhs/math.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/terrain.hpp>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;
using namespace window;

namespace
{

void
kill_entity(common::Logger& logger, TerrainGrid& terrain, TextureTable& ttable,
            EntityRegistry& registry, EntityID const entity_eid)
{
  auto const& bbox = registry.get<AABoundingBox>(entity_eid).cube;
  auto const hw = bbox.half_widths();

  // Move the entity half it's bounding box, so it is directly on the ground, or whatever it was
  // standing on.
  auto& entity_transform = registry.get<Transform>(entity_eid);
  auto& entity_tr = entity_transform.translation;
  entity_tr.y -= hw.y;

  // TODO: Get the normal vector for the terrain at the (x, z) point and rotate the npc to look
  // properly aligned on slanted terrain.
  entity_transform.rotate_degrees(90.0f, X_UNIT_VECTOR);
}

void
try_attack_selected_target(common::Logger& logger, TerrainGrid& terrain, TextureTable& ttable,
                           EntityRegistry& registry, Player &player, EntityID const target_eid)
{
  auto& ptransform      = player.transform();
  auto const playerpos  = ptransform.translation;

  auto& npc_transform = registry.get<Transform>(target_eid);
  auto const npcpos   = npc_transform.translation;

  auto& npcdata = registry.get<NPCData>(target_eid);
  auto& target_hp = npcdata.health;

  bool const already_dead = NPC::is_dead(target_hp);
  if (already_dead) {
    LOG_ERROR("TARGET DEAD");
    return;
  }

  bool const within_attack_range = NPC::within_attack_range(npcpos, playerpos);
  if (within_attack_range) {
    LOG_ERROR("DAMAGING TARGET");
    target_hp.current -= player.damage;
  }
  else {
    LOG_ERROR("TARGET NOT WITHIN ATTACK RANGE");
  }
}

void
move_worldobject(EngineState& es, WorldObject& wo, glm::vec3 const& move_vec,
                 TerrainGrid const& terrain, FrameTime const& ft)
{
  auto& logger = es.logger;
  auto const max_pos = terrain.max_worldpositions();
  auto const max_x   = max_pos.x;
  auto const max_z   = max_pos.y;

  glm::vec3 const delta  = move_vec * wo.speed() * ft.delta_millis();
  glm::vec3 const newpos = wo.world_position() + delta;

  auto const out_of_bounds = terrain.out_of_bounds(newpos.x, newpos.z);
  if (out_of_bounds && !es.mariolike_edges) {
    // If the world object *would* be out of bounds, return early (don't move the WO).
    return;
  }

  auto const flip_sides = [](auto const val, auto const min, auto const max) {
    assert(min < (max - 1));
    auto value = val < min ? max : min;
    return value >= max ? (value - 1) : value;
  };

  if (out_of_bounds.x) {
    auto const new_x = flip_sides(newpos.x, 0ul, max_x);
    wo.move_to(new_x, 0.0, newpos.z);
  }
  else if (out_of_bounds.z) {
    auto const new_z = flip_sides(newpos.z, 0ul, max_z);
    wo.move_to(newpos.x, 0.0, new_z);
  }
  else {
    wo.move(delta);
  }
}

void
update_position(EngineState& es, ZoneState& zs, FrameTime const& ft)
{
  auto& logger   = es.logger;
  auto& ldata    = zs.level_data;
  auto& terrain  = ldata.terrain;

  auto& registry = zs.registry;
  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);
  auto const& movement = player.movement;

  // Move the player forward along the it's movement direction
  auto move_dir = movement.forward
    + movement.backward
    + movement.left
    + movement.right
    + movement.mouse_forward;

  if (move_dir != math::constants::ZERO) {
    move_dir = glm::normalize(move_dir);
  }
  move_worldobject(es, player.world_object, move_dir, terrain, ft);

  // Lookup the player height from the terrain at the player's X, Z world-coordinates.
  auto& player_pos = player.transform().translation;
  float const player_height = terrain.get_height(logger, player_pos.x, player_pos.z);
  auto const& player_bbox   = player.bounding_box().cube;
  player_pos.y              = player_height + player_bbox.half_widths().y;
}

} // namespace

namespace boomhs
{

static auto const HOW_OFTEN_GCD_RESETS_MS = TimeConversions::seconds_to_millis(1);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Player
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

  // Add ourselves to this list of the item's previous owners.
  item.add_owner(this->name);
}

void
Player::drop_entity(common::Logger& logger, EntityID const eid, EntityRegistry& registry)
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

void
Player::update(EngineState& es, ZoneState& zs, FrameTime const& ft)
{
  auto& logger   = es.logger;
  auto& registry = zs.registry;
  auto& ldata    = zs.level_data;
  auto& terrain  = ldata.terrain;
  auto& nbt      = ldata.nearby_targets;

  auto& ttable    = zs.gfx_state.texture_table;

  gcd.update();
  update_position(es, zs, ft);

  // If no target is selected, no more work to do.
  auto const target_opt = nbt.selected();
  if (!target_opt) {
    return;
  }

  bool const gcd_ready = gcd.is_ready();
  auto const reset_gcd_if_ready = [&]() {
    if (gcd_ready) {
      LOG_ERROR_SPRINTF("RESETTING GCD");
      gcd.reset_ms(HOW_OFTEN_GCD_RESETS_MS);
    }
  };
  ON_SCOPE_EXIT(reset_gcd_if_ready);

  // Assumed nearby-target selected
  assert(*target_opt);
  auto const target_eid = *target_opt;
  auto const& target = registry.get<NPCData>(target_eid);

  if (is_attacking && NPC::is_dead(target.health)) {
    is_attacking = false;
    LOG_ERROR_SPRINTF("NOT ATTACKING CAUSE TARGET DEAD");
    return;
  }

  if (is_attacking && gcd_ready) {
    LOG_ERROR_SPRINTF("GCD_READY IS_ATTACKING: %i, GCD_READY: %i", is_attacking, gcd_ready);

    auto& npcdata = registry.get<NPCData>(target_eid);
    auto& target_hp = npcdata.health;

    bool const already_dead = NPC::is_dead(target_hp);
    try_attack_selected_target(logger, terrain, ttable, registry, *this, target_eid);

    bool const target_dead_after_attack = NPC::is_dead(target_hp);
    bool const dead_from_attack = !already_dead && target_dead_after_attack;

    if (dead_from_attack) {
      kill_entity(logger, terrain, ttable, registry, target_eid);
      LOG_ERROR("KILLING TARGET");
    }
    else if (already_dead) {
      LOG_ERROR("TARGET ALREADY DEAD");
    }
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
