#include <boomhs/start_area_generator.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>

#include <opengl/lighting.hpp>
#include <stlw/random.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

void
create_area(TileGrid &tgrid)
{

}

EntityID
place_torch(TileGrid const& tilegrid, EntityRegistry &registry, stlw::float_generator &rng)
{
  auto eid = registry.create();
  registry.assign<Torch>(eid);

  auto &isv = registry.assign<IsVisible>(eid);
  isv.value = true;

  auto &pointlight = registry.assign<PointLight>(eid);
  pointlight.light.diffuse = LOC::YELLOW;

  auto &flicker = registry.assign<LightFlicker>(eid);
  flicker.base_speed = 1.0f;
  flicker.current_speed = flicker.base_speed;

  flicker.colors[0] = LOC::RED;
  flicker.colors[1] = LOC::YELLOW;

  auto &att = pointlight.attenuation;
  att.constant = 1.0f;
  att.linear = 0.93f;
  att.quadratic = 0.46f;

  auto &torch_transform = registry.assign<Transform>(eid);

  auto const pos = TilePosition{2, 2};
  torch_transform.translation = glm::vec3{pos.x, 0.5, pos.y};
  std::cerr << "torchlight pos: '" << torch_transform.translation << "'\n";

  auto &mesh = registry.assign<MeshRenderable>(eid);
  mesh.name = "O_no_normals";

  auto &sn = registry.assign<ShaderName>(eid);
  sn.value = "light";

  return eid;
}

} // ns anon

namespace boomhs
{

LevelGeneredData
StartAreaGenerator::gen_level(EntityRegistry &registry, stlw::float_generator &rng)
{
  std::cerr << "generating starting area ...\n";
  TileGrid tilegrid{20, 20, registry};
  floodfill(tilegrid, TileType::FLOOR);

  auto const set_wall = [&tilegrid](TilePosition const& tpos) {
    tilegrid.data(tpos).type = TileType::WALL;
  };
  tilegrid.visit_edges(set_wall);


  // TODO: turn this into some prefab we can put in both levels (or prefabs that both have teleport
  // tiles guaranteed?)
  tilegrid.data(0, 3).type = TileType::TELEPORTER;

  tilegrid.data(1, 2).type = TileType::WALL;
  tilegrid.data(1, 3).type = TileType::DOOR;
  tilegrid.data(1, 4).type = TileType::WALL;

  // for now no rivers
  std::vector<RiverInfo> rivers;

  auto const starting_pos = TilePosition{10, 10};
  auto const torch_eid = place_torch(tilegrid, registry, rng);

  std::cerr << "finished!\n";
  return LevelGeneredData{MOVE(tilegrid), starting_pos, MOVE(rivers), torch_eid};
}

} // ns boomhs
