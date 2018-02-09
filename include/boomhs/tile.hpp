#pragma once
#include <glm/glm.hpp>
#include <ostream>

namespace boomhs
{

struct TilePosition
{
  using ValueT = int;

  ValueT x = 0, y = 0;

  static TilePosition
  from_floats_truncated(float const x, float const y)
  {
    auto const xx = static_cast<int>(x);
    auto const yy = static_cast<int>(y);
    return TilePosition{xx, yy};
  }
};
inline bool
operator==(TilePosition const& a, TilePosition const& b)
{
  return (a.x == b.x) && (a.y == b.y);
}
inline bool
operator!=(TilePosition const& a, TilePosition const& b)
{
  return !(a == b);
}
inline bool
operator==(TilePosition const& tp, std::pair<TilePosition::ValueT, TilePosition::ValueT> const& pair)
{
  return tp.x == pair.first
    && tp.y == pair.second;
}

glm::vec3
operator+(TilePosition const&, glm::vec3 const&);

std::ostream&
operator<<(std::ostream &, TilePosition const&);

enum class TileType
{
  FLOOR = 0,
  WALL,
  RIVER,
  STAIR_DOWN,
  STAIR_UP,
};

std::ostream&
operator<<(std::ostream &, TileType const);

struct Tile
{
  bool is_visible = false;
  TileType type = TileType::WALL;
  uint32_t eid;

  bool is_stair_up() const { return type == TileType::STAIR_UP; }
  bool is_stair_down() const { return type == TileType::STAIR_DOWN; }

  bool is_stair() const;
};

} // ns boomhs
