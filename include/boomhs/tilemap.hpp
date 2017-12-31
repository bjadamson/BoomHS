#pragma once
#include <vector>
#include <stlw/type_ctors.hpp>

#include <glm/glm.hpp>

namespace boomhs
{

struct Tile {
  bool is_wall = true;
  bool is_visible = false;
};

class TileMap {
  std::vector<Tile> tiles_;
  std::array<std::size_t, 3> dimensions_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(TileMap);

  TileMap(std::vector<Tile> &&t, std::array<std::size_t, 3> const& dimensions)
    : tiles_(MOVE(t))
    , dimensions_(dimensions)
  {
  }
  TileMap(std::vector<Tile> &&t, std::size_t const width, std::size_t const height, std::size_t const length)
    : TileMap(MOVE(t), stlw::make_array<std::size_t>(width, height, length))
  {
  }

  auto
  dimensions() const
  {
    return this->dimensions_;
  }

  inline Tile&
  data(std::size_t const x, std::size_t const y, std::size_t const z)
  {
    auto const [h, w, l] = this->dimensions();
    auto const cell = (z * w * h) + (y * w) + x;
    return this->tiles_[cell];
  }

  inline Tile const&
  data(std::size_t const x, std::size_t const y, std::size_t const z) const
  {
    auto const [h, w, l] = this->dimensions();
    auto const cell = (z * w * h) + (y * w) + x;
    return this->tiles_[cell];
  }

  inline Tile&
  data(glm::vec3 &vec) { return data(vec.x, vec.y, vec.z); }

  inline Tile const&
  data(glm::vec3 const& vec) { return data(vec.x, vec.y, vec.z); }


  inline auto num_tiles() const
  {
    return this->tiles_.size();
  }

  template<typename FN>
  void
  visit_each(FN const& fn) const
  {
    auto const [w, h, l] = dimensions();
    FOR(x, w) {
      FOR(y, h) {
        FOR(z, l) {
          fn(glm::vec3{x, y, z});
        }
      }
    }
  }

  BEGIN_END_FORWARD_FNS(this->tiles_);
};

} // ns boomhs
