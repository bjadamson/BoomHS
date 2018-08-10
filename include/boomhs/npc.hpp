#pragma once
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{
class EntityRegistry;
class RNG;
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

  bool within_attack_range() const;
};

class NPC
{
  NPC() = delete;

public:
  // Loads a new NPC into the EntityRegistry.
  static void create(EntityRegistry&, char const*, int, glm::vec3 const&);

  static void create_random(stlw::Logger&, TerrainGrid const&, EntityRegistry&, RNG&);

  static bool is_dead(HealthPoints const&);
  static bool within_attack_range(glm::vec3 const&, glm::vec3 const&);
};

inline auto
find_enemies(EntityRegistry& registry)
{
  using namespace boomhs;
  using namespace opengl;
  return find_all_entities_with_component<NPCData>(registry);
}

} // namespace boomhs
