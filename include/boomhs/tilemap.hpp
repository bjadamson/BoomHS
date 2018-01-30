#pragma once
#include <stlw/algorithm.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace boomhs
{

struct TilePosition
{
  int x = 0, y = 0, z = 0;
};
inline bool
operator==(TilePosition const& a, TilePosition const& b)
{
  return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

enum class TileType
{
  FLOOR = 0,
  WALL,
  STAIR_WELL,
};

struct StairwellInfo
{
  std::size_t static constexpr NUM_EXITS = 2;
  int tile_pos[NUM_EXITS];
  int exit_pos[NUM_EXITS];
};

struct Tile
{
  bool is_visible = false;
  TileType type = TileType::WALL;
};

class TileMap
{
  std::array<std::size_t, 3> dimensions_;

  std::vector<Tile> tiles_;
  std::vector<StairwellInfo> stairwell_infos_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(TileMap);

  TileMap(std::vector<Tile> &&t, std::size_t const width, std::size_t const height, std::size_t const length)
    : dimensions_(stlw::make_array<std::size_t>(width, height, length))
    , tiles_(MOVE(t))
  {
  }

  auto
  dimensions() const
  {
    return dimensions_;
  }

  Tile&
  data(std::size_t const x, std::size_t const y, std::size_t const z)
  {
    auto const [h, w, l] = dimensions();
    auto const cell = (z * w * h) + (y * w) + x;
    return tiles_[cell];
  }

  Tile const&
  data(std::size_t const x, std::size_t const y, std::size_t const z) const
  {
    auto const [h, w, l] = dimensions();
    auto const cell = (z * w * h) + (y * w) + x;
    return tiles_[cell];
  }

  Tile&
  data(glm::vec3 &vec) { return data(vec.x, vec.y, vec.z); }

  Tile const&
  data(glm::vec3 const& vec) const { return data(vec.x, vec.y, vec.z); }

  auto
  num_tiles() const
  {
    return tiles_.size();
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

  BEGIN_END_FORWARD_FNS(tiles_);
};


class WorldObject;
void
update_visible_tiles(TileMap &, WorldObject const&, bool const);

} // ns boomhs
