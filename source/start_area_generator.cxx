#include <boomhs/item_factory.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/start_area_generator.hpp>
#include <boomhs/tilegrid.hpp>
#include <boomhs/tilegrid_algorithms.hpp>

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
place_torch(TileGrid const& tilegrid, EntityRegistry& registry, stlw::float_generator& rng,
            TextureTable const& ttable)
{
  auto  eid       = ItemFactory::create_torch(registry, rng, ttable);
  auto& transform = registry.get<Transform>(eid);

  transform.translation = glm::vec3{2, 0.5, 2};
}

Result<stlw::empty_type, std::string>
place_prefabs(stlw::Logger& logger, TileGrid& tgrid, stlw::float_generator& rng)
{
  auto         contents = TRY_MOVEOUT(stlw::read_file("prefabs/test0.prefab"));
  size_t const height   = 1 + std::count(contents.begin(), contents.end(), '\n');
  assert(height > 1);

  std::istringstream reader;
  reader.str(contents);

  std::vector<std::string> buffers;
  buffers.resize(height);

  size_t width = 0;
  FOR(i, height)
  {
    auto& buffer = buffers[i];
    std::getline(reader, buffer);

    auto const& length = buffer.size();
    width              = std::max(width, length);
  }

  // find a location
  auto const [h, w] = tgrid.dimensions();
  auto const xpos   = rng.gen_uint64_range(0, w - width);
  auto const ypos   = rng.gen_uint64_range(0, h - height);

  assert(height == buffers.size());
  FOR(x, width)
  {
    FOR(y, height)
    {
      assert(height == buffers.size());
      char const b = buffers[y][x];
      assert(width == buffers[y].size());

      auto& tile = tgrid.data(xpos + x, ypos + y);
      if (b == '#') {
        tile.type = TileType::WALL;
      }
      else if (b == ' ') {
        tile.type = TileType::FLOOR;
      }
      else {
        LOG_ERROR_SPRINTF("x: %i, y: %i", x, y);
        LOG_ERROR_SPRINTF("error value: %c", b);
        std::abort();
      }
    }
  }
  return Ok(stlw::empty_type{});
}

} // namespace

namespace boomhs
{

LevelGeneredData
StartAreaGenerator::gen_level(stlw::Logger& logger, EntityRegistry& registry,
                              stlw::float_generator& rng, TextureTable const& ttable)
{
  LOG_TRACE("generating starting area ...");
  TileGrid tilegrid{30, 30, registry};
  floodfill(tilegrid, TileType::FLOOR);

  place_prefabs(logger, tilegrid, rng);
  LOG_TRACE("done placing prefabs");

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
  place_torch(tilegrid, registry, rng, ttable);

  LOG_TRACE("finished!");
  return LevelGeneredData{MOVE(tilegrid), starting_pos, MOVE(rivers)};
}

} // namespace boomhs
