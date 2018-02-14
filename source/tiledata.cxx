#include <boomhs/tiledata.hpp>
#include <boomhs/assets.hpp>

namespace boomhs
{

FlowDirection
FlowDirection::find_flow(Tile const& tile, std::vector<FlowDirection> const& flows)
{
  auto const cmp = [&tile](auto const& flow) { return flow.tile == tile; };
  auto const find_it = std::find_if(flows.cbegin(), flows.cend(), cmp);
  assert(find_it != flows.cend());
  return *find_it;
}

TileData::TileData(std::vector<Tile> &&t, size_t const width, size_t const height,
    TileInfos const& tinfos, entt::DefaultRegistry &registry)
  : dimensions_(stlw::make_array<size_t>(width, height))
  , registry_(registry)
  , tiles_(MOVE(t))
{
  for (auto &tile : tiles_) {
    tile.eid = registry_.create();
    registry_.assign<Transform>(tile.eid);
    std::cerr << "Tile eid: '" << tile.eid << "'\n";
  }
}

TileData::TileData(TileData &&other)
  : dimensions_(other.dimensions_)
  , registry_(other.registry_)
  , tiles_(MOVE(other.tiles_))
  , flowdirs_(MOVE(other.flowdirs_))
{
  // "This" instance of the tiledata takes over the responsibility of destroying the entities
  // from the moved-from tiledata.
  //
  // The moved-from TileData should no longer destroy the entities during it's destructor.
  this->destroy_entities_ = other.destroy_entities_;
  other.destroy_entities_ = false;
}

TileData::~TileData()
{
  if (destroy_entities_) {
    for (auto &tile : tiles_) {
      registry_.destroy(tile.eid);
    }
  }
}

Tile&
TileData::data(size_t const x, size_t const y)
{
  auto const [w, h] = dimensions();
  assert(x < w);
  assert(y < h);
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

Tile const&
TileData::data(size_t const x, size_t const y) const
{
  auto const [w, h] = dimensions();
  if (y >= h) {
    std::cerr << "x: '" << x << "' y: '" << y << "'\n";
  }
  assert(x < w);
  assert(y < h);
  auto const cell = (w * y) + x;
  return tiles_[cell];
}


bool
float_compare(float const a, float const b)
{
  return std::fabs(a - b) < std::numeric_limits<float>::epsilon();
}

void
TileData::assign_bridge(Tile &tile)
{
  tile.type = TileType::BRIDGE;

  auto const flow = FlowDirection::find_flow(tile, flowdirs_);

  assert(registry_.has<Transform>(tile.eid));
  auto &transform = registry_.get<Transform>(tile.eid);
  if (float_compare(flow.direction.x, 1.0f)) {
    transform.rotate_degrees(90.0f, opengl::Y_UNIT_VECTOR);
  }
}

void
TileData::assign_river(Tile &tile, glm::vec2 const& flow_dir)
{
  tile.type = TileType::RIVER;
  flowdirs_.emplace_back(FlowDirection{tile, flow_dir});
}

} // ns boomhs
