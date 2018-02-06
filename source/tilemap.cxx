#include <boomhs/tilemap.hpp>

namespace boomhs
{

TileNeighbors::TileNeighbors(size_t const num, std::array<TilePosition, 8> &&n)
  : num_neighbors_(num)
  , neighbors_(MOVE(n))
{
  assert(num_neighbors_ <= neighbors_.size());
}

TilePosition const&
TileNeighbors::operator[](size_t const i) const
{
  assert(i < size());
  assert(num_neighbors_ <= neighbors_.size());
  return neighbors_[i];
}

TileMap::TileMap(std::vector<Tile> &&t, int32_t const width, int32_t const height,
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

TileMap::TileMap(TileMap &&other)
  : dimensions_(other.dimensions_)
  , registry_(other.registry_)
  , tiles_(MOVE(other.tiles_))
{
  // "This" instance of the tilemap takes over the responsibility of destroying the entities
  // from the moved-from tilemap.
  //
  // The moved-from TileMap should no longer destroy the entities during it's destructor.
  this->destroy_entities_ = other.destroy_entities_;
  other.destroy_entities_ = false;
}

TileMap::~TileMap()
{
  if (destroy_entities_) {
    for (auto &tile : tiles_) {
      registry_.destroy(tile.eid);
    }
  }
}

Tile&
TileMap::data(size_t const x, size_t const y)
{
  auto const [w, _] = dimensions();
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

Tile const&
TileMap::data(size_t const x, size_t const y) const
{
  auto const [w, _] = dimensions();
  auto const cell = (w * y) + x;
  return tiles_[cell];
}

} // ns boomhs
