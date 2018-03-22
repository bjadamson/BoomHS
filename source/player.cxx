#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/item.hpp>
#include <boomhs/player.hpp>

using namespace boomhs;
using namespace opengl;

namespace boomhs
{

void
Player::pickup_entity(EntityID const eid, EntityRegistry& registry)
{
  auto& inventory = find_inventory(registry);

  assert(inventory.add_item(eid));
  auto& item = registry.get<Item>(eid);
  item.is_pickedup = true;

  auto& visible = registry.get<IsVisible>(eid);
  visible.value = false;
}

void
Player::drop_entity(stlw::Logger& logger, EntityID const eid, EntityRegistry& registry)
{
  auto& inventory = find_inventory(registry);
  auto& item = registry.get<Item>(eid);
  item.is_pickedup = false;

  auto& visible = registry.get<IsVisible>(eid);
  visible.value = true;

  // Move the dropped item to the player's position
  auto const  player_eid = find_player(registry);
  auto const& player_pos = registry.get<Transform>(player_eid).translation;

  auto& transform = registry.get<Transform>(eid);
  transform.translation = player_pos;

  if (registry.has<Torch>(eid))
  {
    auto& pointlight = registry.get<PointLight>(eid);
    pointlight.attenuation *= 3.0f;
    LOG_INFO("You have droppped a torch.");
  }
}

} // namespace boomhs
