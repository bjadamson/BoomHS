#include <boomhs/tiledata.hpp>

namespace boomhs
{

TileData::TileData(std::vector<Tile> &&t, int32_t const width, int32_t const height,
    entt::DefaultRegistry &registry)
  : dimensions_(stlw::make_array<int32_t>(width, height))
  , registry_(registry)
  , tiles_(MOVE(t))
{
  for (auto &tile : tiles_) {
    tile.eid = registry_.create();
    auto &transform = registry_.assign<Transform>(tile.eid);
  }
}

TileData::TileData(TileData &&other)
  : dimensions_(other.dimensions_)
  , registry_(other.registry_)
  , tiles_(MOVE(other.tiles_))
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
  auto const [w, _] = dimensions();
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

Tile const&
TileData::data(size_t const x, size_t const y) const
{
  auto const [w, _] = dimensions();
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

} // ns boomhs
