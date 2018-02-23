#pragma once
#include <stlw/type_macros.hpp>
#include <entt/entt.hpp>

namespace boomhs
{

struct EntityRegistry
{
  entt::DefaultRegistry registry;

  EntityRegistry() = default;
  MOVE_CONSTRUCTIBLE_ONLY(EntityRegistry);
};

} // ns boomhs
