#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{
class float_generator;
} // namespace stlw

namespace boomhs
{
class EntityRegistry;
class TerrainGrid;

enum class Alignment
{
  EVIL = 0,
  NEUTRAL,
  GOOD,
  NOT_SET
};

char const*
alignment_to_string(Alignment const);

struct NPCData
{
  char const*  name      = "NOT SET. ERROR";
  HealthPoints health    = {-1, -1};
  int          level     = -1;
  Alignment    alignment = Alignment::NOT_SET;
};

class NPC
{
  NPC() = delete;

public:
  // Loads a new NPC into the EntityRegistry.
  static void create(EntityRegistry&, char const*, int, glm::vec3 const&);

  static void
  create_random(stlw::Logger&, TerrainGrid const&, EntityRegistry&, stlw::float_generator&);

  static bool is_dead(HealthPoints const&);
};

inline auto
find_enemies(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<NPCData>(registry);
}

} // namespace boomhs
