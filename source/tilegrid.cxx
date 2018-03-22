#include <boomhs/entity.hpp>
#include <boomhs/tilegrid.hpp>

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

TileGrid::TileGrid(size_t const width, size_t const height, EntityRegistry& registry)
    : dimensions_(stlw::make_array<size_t>(width, height))
    , registry_(registry)
{
  FOR(i, width * height)
  {
    Tile tile;
    tile.eid = registry_.create();
    registry_.assign<Transform>(tile.eid);
    registry_.assign<TileComponent>(tile.eid);
    auto& isv = registry_.assign<IsVisible>(tile.eid);
    isv.value = true;
    tiles_.emplace_back(MOVE(tile));
  }
}

TileGrid::TileGrid(TileGrid&& other)
    : dimensions_(other.dimensions_)
    , registry_(other.registry_)
    , tiles_(MOVE(other.tiles_))
    , flowdirs_(MOVE(other.flowdirs_))
{
  // "This" instance of the tilegrid takes over the responsibility of destroying the entities
  // from the moved-from tilegrid.
  //
  // The moved-from TileGrid should no longer destroy the entities during it's destructor.
  this->destroy_entities_ = other.destroy_entities_;
  other.destroy_entities_ = false;
}

TileGrid::~TileGrid()
{
  if (destroy_entities_)
  {
    for (auto& tile : tiles_)
    {
      registry_.destroy(tile.eid);
    }
  }
}

Tile&
TileGrid::data(size_t const x, size_t const y)
{
  auto const [w, h] = dimensions();
  assert(x < w);
  assert(y < h);
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

Tile const&
TileGrid::data(size_t const x, size_t const y) const
{
  auto const [w, h] = dimensions();
  assert(x < w);
  assert(y < h);
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

void
TileGrid::assign_bridge(Tile& tile)
{
  tile.type = TileType::BRIDGE;

  auto const flow = FlowDirection::find_flow(tile, flowdirs_);

  assert(registry_.has<Transform>(tile.eid));
  auto& transform = registry_.get<Transform>(tile.eid);
  if (stlw::math::float_compare(flow.direction.x, 1.0f))
  {
    transform.rotate_degrees(90.0f, opengl::Y_UNIT_VECTOR);
  }
}

void
TileGrid::assign_river(Tile& tile, glm::vec2 const& flow_dir)
{
  tile.type = TileType::RIVER;
  flowdirs_.emplace_back(FlowDirection{tile, flow_dir});
}

bool
TileGrid::is_blocked(TilePosition::ValueT const x, TilePosition::ValueT const y) const
{
  auto const type = data(x, y).type;
  if (ANYOF(type == TileType::WALL, type == TileType::STAIR_DOWN, type == TileType::STAIR_UP))
  {
    return true;
  }
  return false;
}

bool
TileGrid::is_blocked(TilePosition const& tpos) const
{
  return is_blocked(tpos.x, tpos.y);
}

bool
TileGrid::is_visible(Tile& tile)
{
  return tile.is_visible(registry_);
}

void
TileGrid::set_isvisible(Tile& tile, bool const v)
{
  tile.set_isvisible(v, registry_);
}

} // namespace boomhs
