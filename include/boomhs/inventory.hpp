#pragma once
#include <boomhs/item.hpp>

#include <array>

namespace boomhs
{

class InventorySlot
{
  Item *item_ = nullptr;

  void access_assert() const;
public:
  InventorySlot() = default;

  bool occupied() const;

  Item& get();
  Item const& get() const;

  char const*
  name() const;

  operator bool() const { return item_ != nullptr; }

  Item*
  operator->();

  Item const*
  operator->() const;

  // Resets the slot's state to empty.
  void reset();

  // Sets the slot to the item.
  void set(Item &);
};

class Inventory
{
  size_t static constexpr MAX_ITEMS = 40;
  std::array<InventorySlot, MAX_ITEMS> slots_;
  bool open_ = false;

public:
  Inventory() = default;

  using ItemIndex = size_t;

  // Whether or not the game considers the inventory "open" by the player.
  bool is_open() const;
  void toggle_open();

  // Yields references to the slots containing the Item components.
  InventorySlot&
  slot(ItemIndex);

  InventorySlot const&
  slot(ItemIndex) const;

  // Adds an item to the next available slot, returns false if no slots are available.
  //
  // Returns true in all other cases.
  bool add_item(Item &);

  // Sets the item at the given index.
  void set_item(ItemIndex, Item&);

  // Remove the item from the slot
  void remove_item(ItemIndex);
};

} // ns boomhs
