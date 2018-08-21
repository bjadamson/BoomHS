#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/item.hpp>
#include <common/type_macros.hpp>
#include <opengl/texture.hpp>

#include <string>
#include <vector>

namespace opengl
{
struct TextureInfo;
} // namespace opengl

namespace boomhs
{

struct Book
{
};

struct Weapon
{
};

class PreviousOwners
{
  std::vector<Name> values_;

public:
  void add(char const*);
  void add(std::string const&);
  void add(Name const&);

  bool contains(char const*) const;
  bool contains(std::string const&) const;

  COMMON_WRAPPING_CONTAINER_FNS(values_);
};

struct Item
{
  char const*          name        = "UNNAMED";
  char const*          tooltip     = "TOOLTIP NOT SET";
  bool                 is_pickedup = false;
  opengl::TextureInfo* ui_tinfo;

private:
  PreviousOwners owners_;

public:
  void add_owner(char const* v) { owners_.add(v); }
  void add_owner(std::string const& v) { add_owner(v.c_str()); }
  void add_owner(Name const& n) { add_owner(n.value); }

  // By definition, the "last" owner is the
  auto const& current_owner() const
  {
    assert(is_currently_owned());
    return owners_[0];
  }
  auto const& all_owners() { return owners_; }

  bool ever_owned_by(char const*) const;
  bool was_previously_owned() const;
  bool is_currently_owned() const { return !owners_.empty(); }
  bool has_single_owner() const { return 1 == owners_.size(); }
};

inline auto
find_items(EntityRegistry& registry)
{
  std::vector<EntityID> items;
  auto                  view = registry.view<Item>();
  for (auto const eid : view) {
    items.emplace_back(eid);
  }
  return items;
}

} // namespace boomhs
