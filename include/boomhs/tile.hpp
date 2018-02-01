#pragma once
#include <glm/glm.hpp>

#include <ostream>
#include <string>

namespace boomhs
{

struct TilePosition
{
  using ValueT = int;

  ValueT x = 0, z = 0;
};
inline bool
operator==(TilePosition const& a, TilePosition const& b)
{
  return (a.x == b.x) && (a.z == b.z);
}
inline bool
operator!=(TilePosition const& a, TilePosition const& b)
{
  return !(a == b);
}
inline glm::vec3
operator+(TilePosition const& tp, glm::vec3 const& pos)
{
  auto const x = tp.x + pos.x;
  auto const y = 0.0f + pos.y;
  auto const z = tp.z + pos.z;
  return glm::vec3{x, y, z};
}
inline std::ostream&
operator<<(std::ostream &stream, TilePosition const& tp)
{
  stream << "{";
  stream << std::to_string(tp.x);
  stream << ", ";
  stream << std::to_string(tp.z);
  stream << "}";
  return stream;
}

enum class TileType
{
  FLOOR = 0,
  WALL,
  STAIRS,
};
inline std::ostream&
operator<<(std::ostream &stream, TileType const type)
{
  switch(type) {
    case TileType::FLOOR:
      stream << "FLOOR";
      break;
    case TileType::WALL:
      stream << "WALL";
      break;
    case TileType::STAIRS:
      stream << "STAIRS";
      break;
    default:
      std::abort();
      break;
  }
  return stream;
}

struct Tile
{
  bool is_visible = false;
  TileType type = TileType::WALL;
  uint32_t eid;
};

} // ns boomhs
