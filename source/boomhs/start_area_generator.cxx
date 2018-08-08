#include <boomhs/item_factory.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/player.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/terrain.hpp>

#include <algorithm>
#include <opengl/constants.hpp>
#include <boomhs/lighting.hpp>
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
place_item_on_ground(stlw::Logger& logger, TerrainGrid& terrain, Transform &transform,
    glm::vec2 const& pos)
{
  float const x = pos.x, z = pos.y;
  auto const y = terrain.get_height(logger, x, z);
  transform.translation = glm::vec3{x, y, z};
}

void
place_torch(stlw::Logger& logger, TerrainGrid& terrain, EntityRegistry& registry,
            TextureTable& ttable, glm::vec2 const& pos)
{
  auto const eid = ItemFactory::create_torch(registry, ttable);

  auto& transform = registry.get<Transform>(eid);
  place_item_on_ground(logger, terrain, transform, pos);
}

void
place_monsters(stlw::Logger& logger, TerrainGrid const& terrain, EntityRegistry& registry,
               stlw::float_generator& rng)
{
  auto const num_monsters = rng.gen_int_range(MIN_MONSTERS_PER_FLOOR, MAX_MONSTERS_PER_FLOOR);
  FORI(i, num_monsters) { NPC::create_random(logger, terrain, registry, rng); }
}

auto&
place_player(stlw::Logger& logger, TerrainGrid const& terrain,
             MaterialTable const& material_table, EntityRegistry& registry)
{
  auto const eid = registry.create();
  auto& transform = registry.assign<Transform>(eid);

  {
    float const x = 8, z = 8;
    auto const y = terrain.get_height(logger, x, z);
    transform.translation = glm::vec3{x, y, z};
  }

  auto& isv = registry.assign<IsVisible>(eid);
  isv.value = true;

  auto& sn = registry.assign<ShaderName>(eid);
  sn.value = "3d_pos_normal_color";

  auto& meshc = registry.assign<MeshRenderable>(eid);
  meshc.name  = "at";

  auto& cc = registry.assign<Color>(eid);
  cc.set(LOC::WHITE);

  registry.assign<Material>(eid) = material_table.find("player");
  auto& player = registry.assign<Player>(eid);
  {
    auto& wo = player.world_object;
    wo.set_eid(eid);
    wo.set_registry(registry);

    wo.set_forward(-Z_UNIT_VECTOR);
    wo.set_up(Y_UNIT_VECTOR);

    wo.set_speed(460);
  }
  player.level = 14;
  player.name = "BEN";

  return transform;
}

void
place_waters(stlw::Logger& logger, ShaderPrograms& sps, EntityRegistry& registry,
            TextureTable& ttable, stlw::float_generator& rng)
{
  auto& water_sp  = graphics_mode_to_water_shader(GameGraphicsMode::Basic, sps);

  auto const place = [&](glm::vec2 const& pos, unsigned int count) {
    auto const eid = registry.create();

    auto& wi        = WaterFactory::make_default(logger, sps, ttable, eid, registry);
    wi.position     = pos;
    wi.mix_color    = Color::random(rng);
    wi.dimensions   = glm::vec2{20};
    wi.num_vertexes = 4;

    registry.assign<ShaderName>(eid);
    registry.assign<IsVisible>(eid).value = true;

    auto& tr         = registry.assign<Transform>(eid);

    auto const& dimensions = wi.dimensions;
    tr.translation.x = pos.x + dimensions.x / 2.0f;
    tr.translation.z = pos.y + dimensions.y / 2.0f;

    tr.scale.x = dimensions.x;
    tr.scale.z = dimensions.y;

    auto& name = registry.assign<Name>(eid);
    name.value = "WaterInfo #" + std::to_string(count);
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
StartAreaGenerator::gen_level(stlw::Logger& logger, EntityRegistry& registry,
                              stlw::float_generator& rng, ShaderPrograms& sps, TextureTable& ttable,
                              MaterialTable const& material_table,
                              Heightmap const& heightmap)
{
  LOG_TRACE("Generating Starting Area");

  LOG_TRACE("Generating Terrain");
  TerrainConfig const tc;
  auto& sp     = sps.ref_sp(tc.shader_name);

  TerrainGridConfig const tgc;
  auto terrain = terrain::generate_grid(logger, tgc, tc, heightmap, sp);

  LOG_TRACE("Placing Torch");
  place_torch(logger, terrain, registry, ttable, glm::vec2{2, 2});

  LOG_TRACE("Placing Player");
  place_player(logger, terrain, material_table, registry);

  LOG_TRACE("placing monsters ...\n");
  place_monsters(logger, terrain, registry, rng);

  LOG_TRACE("placing water ...\n");
  place_waters(logger, sps, registry, ttable, rng);

  LOG_TRACE("finished generating starting area!");
  return LevelGeneratedData{MOVE(terrain)};
}

} // namespace boomhs
