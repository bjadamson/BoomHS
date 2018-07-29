#include <boomhs/item_factory.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/player.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/terrain.hpp>

#include <algorithm>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <stlw/os.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
using namespace opengl;

static auto constexpr MIN_MONSTERS_PER_FLOOR = 15;
static auto constexpr MAX_MONSTERS_PER_FLOOR = 30;

namespace
{

void
place_torch(stlw::Logger& logger, TerrainGrid& tgrid, EntityRegistry& registry,
            stlw::float_generator& rng, TextureTable& ttable)
{
  auto  eid       = ItemFactory::create_torch(registry, rng, ttable);
  auto& transform = registry.get<Transform>(eid);

  float const x = 2, z = 2;
  auto const y = tgrid.get_height(logger, x, z);
  transform.translation = glm::vec3{x, y, z};
}

void
place_monsters(stlw::Logger& logger, TerrainGrid const& tgrid, EntityRegistry& registry,
               stlw::float_generator& rng)
{
  auto const num_monsters = rng.gen_int_range(MIN_MONSTERS_PER_FLOOR, MAX_MONSTERS_PER_FLOOR);
  FORI(i, num_monsters) { NPC::create_random(logger, tgrid, registry, rng); }
}

} // namespace

namespace boomhs
{

LevelGeneratedData
StartAreaGenerator::gen_level(stlw::Logger& logger, EntityRegistry& registry,
                              stlw::float_generator& rng, ShaderPrograms& sps, TextureTable& ttable,
                              Heightmap const& heightmap)
{
  LOG_TRACE("Generating Starting Area");

  LOG_TRACE("Generating Terrain");
  TerrainConfig const tc;
  auto& sp     = sps.ref_sp(tc.shader_name);

  TerrainGridConfig const tgc;
  auto terrain = terrain::generate_grid(logger, tgc, tc, heightmap, sp);

  LOG_TRACE("Placing Torch");
  place_torch(logger, terrain, registry, rng, ttable);

  auto const peid = find_player(registry);
  auto& player = registry.get<PlayerData>(peid);
  player.level = 14;

  LOG_TRACE("placing monsters ...\n");
  place_monsters(logger, terrain, registry, rng);

  LOG_TRACE("finished generating starting area!");
  return LevelGeneratedData{MOVE(terrain)};
}

} // namespace boomhs
