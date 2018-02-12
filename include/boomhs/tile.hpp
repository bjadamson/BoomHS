#pragma once
#include <stlw/math.hpp>
#include <ostream>

namespace boomhs
{

enum class TileType
{
  FLOOR = 0,
  WALL,
  RIVER,
  BRIDGE,
  STAIR_DOWN,
  STAIR_UP,
};

std::ostream&
operator<<(std::ostream &, TileType const);

struct TilePosition
{
  using ValueT = uint64_t;

  ValueT x = 0, y = 0;

  static TilePosition
  from_floats_truncated(float const x, float const y)
  {
    assert(x >= 0.0f);
    assert(y >= 0.0f);
    auto const xx = static_cast<uint64_t>(x);
    auto const yy = static_cast<uint64_t>(y);
    return TilePosition{xx, yy};
  }

  // Apparently implicit conversion FNS must be a non-static member fns.
  //
  // It's OK to do this conversion implicitely as we don't loose any information going from
  // integers (albeit unsigned) to floating point values.
  operator glm::vec2() const
  {
    return glm::vec2{x, y};
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
