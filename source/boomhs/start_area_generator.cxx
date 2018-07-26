#include <boomhs/item_factory.hpp>
#include <boomhs/heightmap.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/terrain.hpp>

#include <algorithm>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <stlw/os.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

void
place_torch(EntityRegistry& registry, stlw::float_generator& rng,
            TextureTable& ttable)
{
  auto  eid       = ItemFactory::create_torch(registry, rng, ttable);
  auto& transform = registry.get<Transform>(eid);

  transform.translation = glm::vec3{2, 0.5, 2};
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
  place_torch(registry, rng, ttable);


  LOG_TRACE("finished generating starting area!");
  return LevelGeneratedData{MOVE(terrain)};
}

} // namespace boomhs
