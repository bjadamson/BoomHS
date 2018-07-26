#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/terrain.hpp>

#include <stlw/random.hpp>

using namespace boomhs;

namespace
{

glm::vec3
generate_npc_position(TerrainGrid const& terrain_grid, EntityRegistry& registry,
                      stlw::float_generator& rng)
{
  auto const& dimensions = terrain_grid.config.dimensions;
  auto const width      = dimensions.x;
  auto const height     = dimensions.y;
  assert(width > 0 && height > 0);
  uint64_t x, y;
  while (true) {
    x = rng.gen_int_range(0, width - 1);
    y = rng.gen_int_range(0, height - 1);

    glm::vec3 const pos{x, 0, y};
    static auto constexpr MAX_DISTANCE = 2.0f;
    auto const nearby                  = all_nearby_entities(pos, MAX_DISTANCE, registry);
    if (!nearby.empty()) {
      continue;
    }
    return glm::vec3{x, 0, y};
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
NPC::create(EntityRegistry& registry, char const* name, glm::vec3 const& pos)
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
  transform.translation = glm::vec3{pos.x, 0.5, pos.y};

  // npc TAG
  auto& npcdata     = registry.assign<NPCData>(eid);
  npcdata.name      = name;
  npcdata.health    = 10;
  npcdata.level     = 3;
  npcdata.alignment = Alignment::EVIL;

  // visible
  auto& isv = registry.assign<IsVisible>(eid);
  isv.value = true;
}

void
NPC::create_random(TerrainGrid const& terrain_grid, EntityRegistry& registry, stlw::float_generator& rng)
{
  auto const make_monster = [&](char const* name) {
    auto const pos = generate_npc_position(terrain_grid, registry, rng);
    NPC::create(registry, name, pos);
  };
  if (rng.gen_bool()) {
    make_monster("O");
  }
  else {
    make_monster("T");
  }
}

} // namespace boomhs
