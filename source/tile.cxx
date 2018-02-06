#include <boomhs/tile.hpp>
#include <stlw/debug.hpp>
#include <string>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TilePosition
glm::vec3
operator+(TilePosition const& tp, glm::vec3 const& pos)
{
  auto const x = tp.x + pos.x;
  auto const y = 0.0f + pos.y;
  auto const z = tp.z + pos.z;
  return glm::vec3{x, y, z};
}

std::ostream&
operator<<(std::ostream &stream, TilePosition const& tp)
{
  stream << "{";
  stream << std::to_string(tp.x);
  stream << ", ";
  stream << std::to_string(tp.z);
  stream << "}";
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TileType
std::ostream&
operator<<(std::ostream &stream, TileType const type)
{
  switch(type) {
    case TileType::FLOOR:
      stream << "FLOOR";
      break;
    case TileType::WALL:
      stream << "WALL";
      break;
    case TileType::STAIR_DOWN:
      stream << "STAIR_DOWN";
      break;
    case TileType::STAIR_UP:
      stream << "STAIR_UP";
      break;
    default:
      std::abort();
      break;
  }
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Tile
bool
Tile::is_stair() const
{
  stlw::assert_exactly_only_one_true(is_stair_up(), is_stair_down());
  return is_stair_up() || is_stair_down();
}

} // ns boomhs
