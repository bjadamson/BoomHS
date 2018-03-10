#include <boomhs/inventory.hpp>
#include <iostream>

namespace
{
} // ns anon

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// InventorySlot
void
InventorySlot::access_assert() const
{
  assert(occupied());
}

bool
InventorySlot::occupied() const
{
  return nullptr != item_;
}

char const*
InventorySlot::name() const
{
  return occupied() ? item_->name : "Slot Unoccupied";
}

void
InventorySlot::reset()
{
  item_ = nullptr;
}

void
InventorySlot::set(Item &item)
{
  item_ = &item;
}

Item&
InventorySlot::get()
{
  access_assert();
  return *item_;
}

Item const&
InventorySlot::get() const
{
  access_assert();
  return *item_;
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
Inventory::add_item(Item &item_adding)
{
  FOR(i, MAX_ITEMS) {
    auto const& item_at_index = slot(i);
    if (item_at_index) {
      continue;
    }
    // found an open slot !!
    set_item(i, item_adding);
    return true;
  }
  // no open slot found.
  return false;
}

void
Inventory::set_item(ItemIndex const i, Item &it)
{
  slot(i).set(it);
}

void
Inventory::remove_item(ItemIndex const i)
{
  slot(i).reset();
}

Item*
InventorySlot::operator->()
{
  return &get();
}

Item const*
InventorySlot::operator->() const
{
  return &get();
}

} // ns boomhs
