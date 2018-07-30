#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/terrain.hpp>

#include <stlw/random.hpp>

using namespace boomhs;

namespace
{

glm::vec3
generate_npc_position(stlw::Logger& logger, TerrainGrid const& terrain_grid,
                      EntityRegistry& registry, stlw::float_generator& rng)
{
  auto const& dimensions = terrain_grid.max_worldpositions();
  auto const width      = dimensions.x;
  auto const length     = dimensions.y;
  assert(width > 0 && length > 0);
  float x, z;
  while (true) {
    x = rng.gen_float_range(0, width - 1);
    z = rng.gen_float_range(0, length - 1);

    float const y = terrain_grid.get_height(logger, x, z);

    glm::vec3 const pos{x, y, z};
    static auto constexpr MAX_DISTANCE = 2.0f;
    auto const nearby                  = all_nearby_entities(pos, MAX_DISTANCE, registry);
    if (!nearby.empty()) {
      continue;
    }
    return pos;
  }

  std::abort();
  return glm::vec3{0};
}

} // namespace

namespace boomhs
{

char const*
alignment_to_string(Alignment const al)
{
#define CASE(ATTRIBUTE, ATTRIBUTE_S)                                                               \
  case Alignment::ATTRIBUTE:                                                                       \
    return ATTRIBUTE_S;

  switch (al) {
    CASE(EVIL, "EVIL");
    CASE(NEUTRAL, "NEUTRAL");
    CASE(GOOD, "GOOD");
    CASE(NOT_SET, "NOT_SET");
  default:
    break;
  }
#undef CASE

  // terminal error
  std::abort();
}

void
NPC::create(EntityRegistry& registry, char const* name, int const level, glm::vec3 const& pos)
{
  auto eid = registry.create();
  registry.assign<opengl::Color>(eid);
  registry.assign<Name>(eid).value = name;

  // Enemies get a mesh
  auto& meshc = registry.assign<MeshRenderable>(eid);
  meshc.name  = name;

  // shader
  auto& sn = registry.assign<ShaderName>(eid);
  sn.value = "3d_pos_normal_color";

  // transform
  auto& transform       = registry.assign<Transform>(eid);
  transform.translation = pos;

  // npc TAG
  auto& npcdata     = registry.assign<NPCData>(eid);
  npcdata.name      = name;

  auto& hp   = npcdata.health;
  hp.current = 10;
  hp.max     = hp.current;

  npcdata.level     = level;
  npcdata.alignment = Alignment::EVIL;

  // visible
  auto& isv = registry.assign<IsVisible>(eid);
  isv.value = true;
}

void
NPC::create_random(stlw::Logger& logger, TerrainGrid const& terrain_grid, EntityRegistry& registry,
    stlw::float_generator& rng)
{
  auto const make_monster = [&](char const* name) {
    auto const pos = generate_npc_position(logger, terrain_grid, registry, rng);

    int const level = rng.gen_int_range(1, 20);
    NPC::create(registry, name, level, pos);
  };
  if (rng.gen_bool()) {
    make_monster("O");
  }
  else {
    make_monster("T");
  }
}

bool
NPC::is_dead(HealthPoints const& hp)
{
  assert(hp.max > 0);
  return hp.current <= 0;
}

} // namespace boomhs
