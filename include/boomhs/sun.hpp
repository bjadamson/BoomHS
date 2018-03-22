#pragma once
#include <boomhs/components.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{

struct Sun
{
};

inline auto&
find_sun(EntityRegistry &registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Sun>().size());

  // Assume Skybox has a Transform
  auto view = registry.view<Sun, Transform>();
  std::optional<EntityID> entity{std::nullopt};
  for (auto const e : view) {
    // This assert ensures this loop only runs once.
    assert(std::nullopt == entity);
    entity = e;
  }
  assert(std::nullopt != entity);
  return *entity;
}

} // ns boomhs
