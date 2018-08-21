#include <boomhs/item_factory.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/math.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/lighting.hpp>
#include <boomhs/player.hpp>
#include <boomhs/random.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/terrain.hpp>

#include <opengl/texture.hpp>

#include <common/os.hpp>

#include <algorithm>

using namespace boomhs;
using namespace boomhs::math::constants;
using namespace opengl;

static auto constexpr MIN_MONSTERS_PER_FLOOR = 15;
static auto constexpr MAX_MONSTERS_PER_FLOOR = 30;

namespace
{

void
place_item_on_ground(common::Logger& logger, TerrainGrid& terrain, Transform &transform,
    glm::vec2 const& pos)
{
  float const x = pos.x, z = pos.y;
  auto const y = terrain.get_height(logger, x, z);
  transform.translation = glm::vec3{x, y, z};
}

void
place_torch(common::Logger& logger, TerrainGrid& terrain, EntityRegistry& registry,
            TextureTable& ttable, glm::vec2 const& pos)
{
  auto const eid = ItemFactory::create_torch(registry, ttable);

  auto& transform = registry.get<Transform>(eid);
  place_item_on_ground(logger, terrain, transform, pos);
}

void
place_monsters(common::Logger& logger, TerrainGrid const& terrain, EntityRegistry& registry,
               RNG& rng)
{
  auto const num_monsters = rng.gen_int_range(MIN_MONSTERS_PER_FLOOR, MAX_MONSTERS_PER_FLOOR);
  FORI(i, num_monsters) { NPC::create_random(logger, terrain, registry, rng); }
}

void
place_player(common::Logger& logger, ShaderPrograms& sps, TerrainGrid const& terrain,
             MaterialTable const& material_table, EntityRegistry& registry)
{
  auto const eid = registry.create();

  auto const FWD     = -Z_UNIT_VECTOR;
  auto constexpr UP  =  Y_UNIT_VECTOR;
  auto& player = registry.assign<Player>(eid, logger, eid, registry, sps, FWD, UP);
  player.level = 14;
  player.name  = "BEN";
  player.speed = 460;

  auto& transform = player.transform();

  {
    float const x = 8, z = 8;
    auto const y = terrain.get_height(logger, x, z);
    transform.translation = glm::vec3{x, y, z};
  }

  registry.assign<IsRenderable>(eid);
  registry.assign<ShaderName>(eid, "3d_pos_normal_color");
  registry.assign<MeshRenderable>(eid, "at");

  auto& cc = registry.assign<Color>(eid);
  cc.set(LOC::WHITE);

  registry.assign<Material>(eid) = material_table.find("player");
}

void
place_waters(common::Logger& logger, ShaderPrograms& sps, EntityRegistry& registry,
            TextureTable& ttable, RNG& rng)
{
  auto& water_sp  = graphics_mode_to_water_shader(GameGraphicsMode::Basic, sps);

  auto const place = [&](glm::vec2 const& pos, unsigned int count) {
    auto const eid = registry.create();

    auto& wi        = WaterFactory::make_default(logger, sps, ttable, eid, registry);
    wi.mix_color    = Color::random(rng);

    auto& tr = registry.get<Transform>(eid);
    tr.translation.x = pos.x;
    tr.translation.z = pos.y;

    auto name = "WaterInfo #" + std::to_string(count);
    registry.assign<Name>(eid, MOVE(name));
  };

  int count = 0;
  FOR(i, 4) {
    FOR(j, 4) {
      auto const pos = glm::vec2{i * 25, j * 25};
      place(pos, count);
      ++count;
    }
  }
}

} // namespace

namespace boomhs
{

LevelGeneratedData
StartAreaGenerator::gen_level(common::Logger& logger, EntityRegistry& registry,
                              RNG& rng, ShaderPrograms& sps, TextureTable& ttable,
                              MaterialTable const& material_table,
                              Heightmap const& heightmap)
{
  LOG_TRACE("Generating Starting Area");

  LOG_TRACE("Generating Terrain");
  TerrainConfig const tc;
  auto& sp     = sps.ref_sp(tc.shader_name);

  TerrainGridConfig tgc;
  tgc.num_rows = 2;
  tgc.num_cols = 2;
  auto terrain = terrain::generate_grid(logger, tgc, tc, heightmap, sp);

  LOG_TRACE("Placing Torch");
  place_torch(logger, terrain, registry, ttable, glm::vec2{2, 2});

  LOG_TRACE("Placing Player");
  place_player(logger, sps, terrain, material_table, registry);

  LOG_TRACE("placing monsters ...\n");
  place_monsters(logger, terrain, registry, rng);

  LOG_TRACE("placing water ...\n");
  place_waters(logger, sps, registry, ttable, rng);

  LOG_TRACE("finished generating starting area!");
  return LevelGeneratedData{MOVE(terrain)};
}

} // namespace boomhs
