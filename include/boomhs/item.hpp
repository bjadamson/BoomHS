#pragma once
#include <boomhs/entity.hpp>
#include <opengl/texture.hpp>
#include <stlw/type_macros.hpp>
#include <vector>

namespace boomhs
{

struct Item
{
  char const*          name        = "UNNAMED";
  char const*          tooltip     = "TOOLTIP NOT SET";
  bool                 is_pickedup = false;
  opengl::TextureInfo* ui_tinfo;
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
