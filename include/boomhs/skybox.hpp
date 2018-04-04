#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/types.hpp>
#include <optional>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct IsSkybox
{
};

inline auto
find_skybox(EntityRegistry& registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<IsSkybox>().size());

  // Assume Skybox has a Transform
  auto                    view = registry.view<IsSkybox, Transform>();
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
