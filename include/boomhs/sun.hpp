#pragma once
#include <boomhs/components.hpp>
#include <extlibs/glm.hpp>

namespace boomhs
{

struct OrbitalBody
{
  float x_radius = 0.0f;
  float z_radius = 0.0f;
};

struct Sun
{
  float max_height;
};

inline auto&
find_sun(EntityRegistry& registry)
{
  // for now assume only 1 entity has the Player tag
  assert(1 == registry.view<Sun>().size());

  // Assume Skybox has a Transform
  auto                    view = registry.view<Sun, Transform>();
  std::optional<EntityID> entity{std::nullopt};
  for (auto const e : view)
  {
    // This assert ensures this loop only runs once.
    assert(std::nullopt == entity);
    entity = e;
  }
  assert(std::nullopt != entity);
  return *entity;
}

} // namespace boomhs
