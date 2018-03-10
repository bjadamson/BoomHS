#include <boomhs/player.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/item.hpp>
#include <boomhs/inventory.hpp>

using namespace boomhs;

namespace
{

auto&
find_inventory(EntityRegistry &registry)
{
  auto const eid = find_player(registry);
  return registry.get<PlayerData>(eid).inventory;
}

} // ns anon

namespace boomhs
{

void
Player::add_item(EntityID const eid, Item &item, EntityRegistry &registry)
{
  auto &inventory = find_inventory(registry);

  assert(inventory.add_item(item));
  item.is_pickedup = true;

  auto &visible = registry.get<IsVisible>(eid);
  visible.value = false;
}

void
Player::remove_item(EntityID const eid, Item &item, EntityRegistry &registry)
{
  std::abort();
  /*
  //inventory.remove_item(item);
  item.is_pickedup = false;

  auto &visible = registry.get<IsVisible>(eid);
  visible.value = true;

  if (registry.has<Torch>(eid)) {
    auto &pointlight = registry.get<PointLight>(eid);
    pointlight.attenuation *= 3.0f;
    std::cerr << "You have droppped a torch.\n";
  }
  */
}

void
Player::remove_item(size_t const index, Item &item, EntityRegistry &registry)
{
  auto &inventory = find_inventory(registry);
  inventory.remove_item(index);

  item.is_pickedup = false;
  /*

  auto &visible = registry.get<IsVisible>(eid);
  visible.value = true;

  if (registry.has<Torch>(eid)) {
    auto &pointlight = registry.get<PointLight>(eid);
    pointlight.attenuation *= 3.0f;
    std::cerr << "You have droppped a torch.\n";
  }
  */
}

} // ns boomhs
