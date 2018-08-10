#include <boomhs/components.hpp>
#include <boomhs/inventory.hpp>
#include <boomhs/item_factory.hpp>

#include <common/algorithm.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// InventorySlot
void
InventorySlot::access_assert() const
{
  assert(occupied());
}

EntityID
InventorySlot::eid() const
{
  access_assert();
  return *eid_;
}

bool
InventorySlot::occupied() const
{
  return std::nullopt != eid_;
}

char const*
InventorySlot::name(EntityRegistry& registry) const
{
  return occupied() ? item(registry).name : "Slot Unoccupied";
}

void
InventorySlot::reset()
{
  eid_ = std::nullopt;
}

void
InventorySlot::set(EntityID const eid)
{
  eid_ = std::make_optional(eid);
}

Item&
InventorySlot::item(EntityRegistry& registry)
{
  access_assert();
  return registry.get<Item>(*eid_);
}

Item const&
InventorySlot::item(EntityRegistry& registry) const
{
  access_assert();
  return registry.get<Item>(*eid_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Inventory
bool
Inventory::is_open() const
{
  return open_;
}

void
Inventory::toggle_open()
{
  open_ ^= true;
}

InventorySlot&
Inventory::slot(ItemIndex const index)
{
  assert(index < MAX_ITEMS);
  return slots_[index];
}

InventorySlot const&
Inventory::slot(ItemIndex const index) const
{
  assert(index < MAX_ITEMS);
  return slots_[index];
}

bool
Inventory::add_item(EntityID const eid)
{
  FOR(i, MAX_ITEMS)
  {
    auto const& item_at_index = slot(i);
    if (item_at_index.occupied()) {
      continue;
    }
    // found an open slot !!
    set_item(i, eid);
    return true;
  }
  // no open slot found.
  return false;
}

void
Inventory::set_item(ItemIndex const i, EntityID const eid)
{
  slot(i).set(eid);
}

void
Inventory::remove_item(ItemIndex const i)
{
  slot(i).reset();
}

} // namespace boomhs
