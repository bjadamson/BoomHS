#include <vector>
#include <stlw/type_ctors.hpp>

#include <glm/glm.hpp>

namespace boomhs
{

struct Tile {
  glm::vec3 const pos;
  bool is_wall = true;
};

class TileMap {
  std::vector<Tile> tiles_;
  unsigned int width_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(TileMap);
  TileMap(std::vector<Tile> &&t, unsigned int width)
    : tiles_(MOVE(t))
    , width_(width)
    {
      assert((tiles_.size() % width_) == 0);
    }

  auto width() const { return this->width_; }

  inline Tile const& data(unsigned int x, unsigned int y) const
  {
    return this->tiles_[x + y * this->width()];
  }

  inline Tile& data(unsigned int x, unsigned int y)
  {
    return this->tiles_[x + y * this->width()];
  }

  inline auto num_tiles() const
  {
    return this->tiles_.size();
  }

  BEGIN_END_FORWARD_FNS(this->tiles_);
};

} // ns boomhs
