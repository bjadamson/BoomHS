#include <boomhs/components.hpp>
#include <boomhs/bounding_object.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/item.hpp>
#include <boomhs/item_factory.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/math.hpp>
#include <boomhs/nearby_targets.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/terrain.hpp>
#include <boomhs/zone_state.hpp>
#include <optional>

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;
using namespace common;
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
  entity_transform.rotate_degrees(90.0f, math::EulerAxis::X);
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
                 float const speed, TerrainGrid const& terrain, FrameTime const& ft)
{
  auto& logger = es.logger;
  auto const max_pos = terrain.max_worldpositions();
  auto const max_x   = max_pos.x;
  auto const max_z   = max_pos.y;

  glm::vec3 const delta  = move_vec * speed * ft.delta_millis();
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
update_position(EngineState& es, LevelData& ldata, Player& player, FrameTime const& ft)
{
  auto& logger   = es.logger;
  auto& terrain  = ldata.terrain;
  auto const& movement = es.movement_state;

  // Move the player forward along it's movement direction
  auto move_dir = movement.forward
    + movement.backward
    + movement.left
    + movement.right
    + movement.mouse_forward;

  if (move_dir != math::constants::ZERO) {
    move_dir = glm::normalize(move_dir);
  }

  auto& wo = player.world_object();
  move_worldobject(es, wo, move_dir, player.speed, terrain, ft);

  // Lookup the player height from the terrain at the player's X, Z world-coordinates.
  auto& player_pos = player.transform().translation;
  float const player_height = terrain.get_height(logger, player_pos.x, player_pos.z);
  auto const& player_bbox   = player.bounding_box().cube;
  player_pos.y              = player_height + player_bbox.half_widths().y;
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// PlayerHead
PlayerHead::PlayerHead(EntityRegistry& registry, EntityID const eid,
    glm::vec3 const& fwd, glm::vec3 const& up)
    : registry_(&registry)
    , eid_(eid)
    , world_object(eid, registry, fwd, up)
{
}

void
PlayerHead::update(FrameTime const& ft)
{
  auto const player_eid = find_player_eid(*registry_);

  auto const& player_bbox = registry_->get<AABoundingBox>(player_eid).cube;
  auto const& head_bbox   = registry_->get<AABoundingBox>(eid_).cube;

  auto const& player_tr = registry_->get<Transform>(player_eid);
  auto& head_tr         = registry_->get<Transform>(eid_);

  auto const player_half_height = player_bbox.scaled_half_widths(player_tr).y;
  auto const head_half_height   = head_bbox.scaled_half_widths(head_tr).y;

  head_tr.translation = player_tr.translation;
  head_tr.translation.y += (player_half_height - head_half_height);
}

PlayerHead
PlayerHead::create(common::Logger& logger, EntityRegistry& registry, ShaderPrograms& sps)
{
  // construct the head
  auto eid = registry.create();
  registry.assign<IsRenderable>(eid);
  registry.assign<Name>(eid, "PlayerHead");

  auto& bbox = AABoundingBox::add_to_entity(logger, sps, eid, registry, -ONE, ONE);

  // The head follows the Player
  auto const player_eid = find_player_eid(registry);
  auto& ft = registry.assign<FollowTransform>(eid, player_eid);
  //ft.target_offset = glm::vec3{0.0f, 0.6f, 0.0f};
  auto& player = find_player(registry).world_object();
  PlayerHead ph{registry, eid, -constants::Z_UNIT_VECTOR, constants::X_UNIT_VECTOR};
  auto& tr = ph.world_object.transform();
  tr.scale = glm::vec3{0.1};

  return ph;
}

static auto const HOW_OFTEN_GCD_RESETS_MS = TimeConversions::seconds_to_millis(1);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Player
Player::Player(common::Logger& logger, EntityID const eid, EntityRegistry& r, ShaderPrograms& sps,
               glm::vec3 const& fwd, glm::vec3 const& up)
    : registry_(&r)
    , eid_(eid)
    , wo_(eid, r, fwd, up)
    , head_(PlayerHead::create(logger, *registry_, sps))
{
}

void
Player::pickup_entity(EntityID const eid, EntityRegistry& registry)
{
  auto& player    = find_player(registry);
  auto& inventory = player.inventory;


  assert(inventory.add_item(eid));
  auto& item       = registry.get<Item>(eid);
  item.is_pickedup = true;

  registry.get<IsRenderable>(eid).hidden = true;

  // Add ourselves to this list of the item's previous owners.
  item.add_owner(this->name);
}


void
Player::drop_entity(common::Logger& logger, EntityID const eid, EntityRegistry& registry)
{
  auto& player    = find_player(registry);
  auto& inventory = player.inventory;

  auto& item       = registry.get<Item>(eid);
  item.is_pickedup = false;

  registry.get<IsRenderable>(eid).hidden = false;

  // Move the dropped item to the player's position
  auto const& player_pos = player.world_object().world_position();

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

  gcd_.update();
  update_position(es, ldata, *this, ft);
  head_.update(ft);

  // If no target is selected, no more work to do.
  auto const target_opt = nbt.selected();
  if (!target_opt) {
    return;
  }

  bool const gcd_ready = gcd_.is_ready();
  auto const reset_gcd_if_ready = [&]() {
    if (gcd_ready) {
      LOG_ERROR_SPRINTF("RESETTING GCD");
      gcd_.reset_ms(HOW_OFTEN_GCD_RESETS_MS);
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

void
Player::try_pickup_nearby_item(common::Logger& logger, EntityRegistry& registry, FrameTime const& ft)
{
  auto const& player_pos       = transform().translation;

  static constexpr auto MINIMUM_DISTANCE_TO_PICKUP = 1.0f;
  auto const            items                      = find_items(registry);
  for (EntityID const eid : items) {
    Item& item = registry.get<Item>(eid);
    if (item.is_pickedup) {
      LOG_INFO("item already picked up.\n");
      continue;
    }

    auto&       item_transform = registry.get<Transform>(eid);
    auto const& item_pos       = item_transform.translation;
    auto const  distance       = glm::distance(item_pos, player_pos);

    if (distance > MINIMUM_DISTANCE_TO_PICKUP) {
      LOG_INFO("There is nothing nearby to pickup.");
      continue;
    }

    pickup_entity(eid, registry);

    if (registry.has<Torch>(eid)) {
      auto& pointlight = registry.get<PointLight>(eid);
      pointlight.attenuation /= 3.0f;

      LOG_INFO("You have picked up a torch.");
    }
    else {
      LOG_INFO("You have picked up an item.");
    }
  }
}

glm::vec3
Player::world_position() const
{
  return transform().translation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
EntityID
find_player_eid(EntityRegistry& registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Player>().size());

  std::optional<EntityID> peid;
  for (auto const eid : registry.view<Player>()) {
    // This assert ensures this loop only runs once.
    assert(!peid);
    peid = eid;
  }
  assert(peid);
  return *peid;
}

Player&
find_player(EntityRegistry& registry)
{
  auto const peid = find_player_eid(registry);
  return registry.get<Player>(peid);
}

} // namespace boomhs
