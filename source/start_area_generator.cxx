#include <boomhs/start_area_generator.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>

#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <stlw/os.hpp>
#include <stlw/random.hpp>
#include <algorithm>

using namespace boomhs;
using namespace opengl;

namespace
{

EntityID
place_torch(TileGrid const& tilegrid, EntityRegistry &registry, stlw::float_generator &rng,
    TextureTable const& ttable)
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
  mesh.name = "O_uvs_no_normals";

  auto &tr = registry.assign<TextureRenderable>(eid);
  auto texture_o = ttable.find("Lava");
  assert(texture_o);
  tr.texture_info = *texture_o;

  auto &sn = registry.assign<ShaderName>(eid);
  sn.value = "light_texture";

  return eid;
}

stlw::result<std::string, std::string>
place_prefabs(TileGrid &tgrid, stlw::float_generator &rng)
{
  DO_TRY(auto contents, stlw::read_file("prefabs/test0.prefab"));
  size_t const height = 1 + std::count(contents.begin(), contents.end(), '\n');
  assert(height > 1);

  std::istringstream reader;
  reader.str(contents);

  std::vector<std::string> buffers;
  buffers.resize(height);

  size_t width = 0;
  FOR(i, height) {
    auto &buffer = buffers[i];
    std::getline(reader, buffer);

    auto const& length = buffer.size();
    width = std::max(width, length);
  }


  // find a location
  auto const [h, w] = tgrid.dimensions();
  auto const xpos = rng.gen_uint64_range(0, w - width);
  auto const ypos = rng.gen_uint64_range(0, h - height);

  assert(height == buffers.size());
  FOR(x, width) {
    FOR(y, height) {
      assert(height == buffers.size());
      char const b = buffers[y][x];
      assert(width == buffers[y].size());

      auto &tile = tgrid.data(xpos + x, ypos + y);
      if (b == '#') {
        tile.type = TileType::WALL;
      } else if(b == ' ') {
        tile.type = TileType::FLOOR;
      } else {
        std::cerr << "x: '" << x << "' , y: '" << y << "'\n";
        std::cerr << "error value: '" << b << "'\n";
        std::abort();
      }
    }
  }
  return std::string{};
}

} // ns anon

namespace boomhs
{

LevelGeneredData
StartAreaGenerator::gen_level(EntityRegistry &registry, stlw::float_generator &rng,
    TextureTable const& ttable)
{
  std::cerr << "generating starting area ...\n";
  TileGrid tilegrid{30, 30, registry};
  floodfill(tilegrid, TileType::FLOOR);

  place_prefabs(tilegrid, rng);
  std::cerr << "done placing prefabs\n";

  auto const set_wall = [&tilegrid](TilePosition const& tpos) {
    tilegrid.data(tpos).type = TileType::WALL;
  };
  visit_edges(tilegrid, set_wall);


  // TODO: turn this into some prefab we can put in both levels (or prefabs that both have teleport
  // tiles guaranteed?)
  tilegrid.data(0, 3).type = TileType::TELEPORTER;

  tilegrid.data(1, 2).type = TileType::WALL;
  tilegrid.data(1, 3).type = TileType::DOOR;
  tilegrid.data(1, 4).type = TileType::WALL;

  // for now no rivers
  std::vector<RiverInfo> rivers;

  auto const starting_pos = TilePosition{10, 10};
  auto const torch_eid = place_torch(tilegrid, registry, rng, ttable);

  std::cerr << "finished!\n";
  return LevelGeneredData{MOVE(tilegrid), starting_pos, MOVE(rivers), torch_eid};
}

} // ns boomhs
