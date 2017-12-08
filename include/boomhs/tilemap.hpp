#include <vector>
#include <stlw/type_ctors.hpp>

#include <glm/glm.hpp>

namespace boomhs
{

struct Tile {
  bool is_wall = true;
};

class TileMap {
  std::vector<Tile> tiles_;
  std::array<std::size_t, 3> dimensions_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(TileMap);
  TileMap(std::vector<Tile> &&t, std::size_t const width, std::size_t const height, std::size_t const length)
    : tiles_(MOVE(t))
    , dimensions_({width, height, length})
  {
  }

  auto dimensions() const
  {
    return this->dimensions_;
  }

  inline Tile const& data(std::size_t const x, std::size_t const y, std::size_t const z) const
  {
    auto const [h, w, l] = this->dimensions();
    auto const cell = (z * w * h) + (y * w) + x;
    return this->tiles_[cell];
  }

  inline Tile& data(std::size_t const x, std::size_t const y, std::size_t const z)
  {
    auto const [h, w, l] = this->dimensions();
    auto const cell = (z * w * h) + (y * w) + x;
    return this->tiles_[cell];
  }

  inline auto num_tiles() const
  {
    return this->tiles_.size();
  }

  BEGIN_END_FORWARD_FNS(this->tiles_);
};

} // ns boomhs
