#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/tile.hpp>
#include <boomhs/tilegrid.hpp>

#include <stlw/random.hpp>

using namespace boomhs;

namespace
{

TilePosition
generate_npc_position(TileGrid const& tilegrid, EntityRegistry& registry,
                      stlw::float_generator& rng)
{
  auto const dimensions = tilegrid.dimensions();
  auto const width = dimensions[0];
  auto const height = dimensions[1];
  assert(width > 0 && height > 0);
  uint64_t x, y;
  while (true)
  {
    x = rng.gen_int_range(0, width - 1);
    y = rng.gen_int_range(0, height - 1);

    if (tilegrid.is_blocked(x, y))
    {
      continue;
    }

    glm::vec3 const pos{x, 0, y};
    static auto constexpr MAX_DISTANCE = 2.0f;
    auto const nearby = all_nearby_entities(pos, MAX_DISTANCE, registry);
    if (!nearby.empty())
    {
      continue;
    }
    return TilePosition{x, y};
  }

  std::abort();
  return TilePosition{0, 0};
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

  switch (al)
  {
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
NPC::create(EntityRegistry& registry, char const* name, TilePosition const& tpos)
{
  auto eid = registry.create();
  registry.assign<opengl::Color>(eid);
  registry.assign<Name>(eid).value = name;

  // Enemies get a mesh
  auto& meshc = registry.assign<MeshRenderable>(eid);
  meshc.name = name;

  // shader
  auto& sn = registry.assign<ShaderName>(eid);
  sn.value = "3d_pos_normal_color";

  // transform
  auto& transform = registry.assign<Transform>(eid);
  transform.translation = glm::vec3{tpos.x, 0.5, tpos.y};

  // npc TAG
  auto& npcdata = registry.assign<NPCData>(eid);
  npcdata.name = name;
  npcdata.health = 10;
  npcdata.level = 3;
  npcdata.alignment = Alignment::EVIL;

  // visible
  auto& isv = registry.assign<IsVisible>(eid);
  isv.value = true;
}

void
NPC::create_random(TileGrid const& tilegrid, EntityRegistry& registry, stlw::float_generator& rng)
{
  auto const make_monster = [&](char const* name) {
    auto const tpos = generate_npc_position(tilegrid, registry, rng);
    NPC::create(registry, name, tpos);
  };
  if (rng.gen_bool())
  {
    make_monster("O");
  }
  else
  {
    make_monster("T");
  }
}

} // namespace boomhs
