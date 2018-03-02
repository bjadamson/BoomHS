#include <boomhs/start_area_generator.hpp>
#include <boomhs/item_factory.hpp>
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
  auto eid = ItemFactory::create_torch(registry, rng, ttable);
  auto &transform = registry.get<Transform>(eid);

  transform.translation = glm::vec3{2, 0.5, 2};
  std::cerr << "torchlight pos: '" << transform.translation << "'\n";

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
