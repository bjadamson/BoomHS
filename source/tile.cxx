#include <boomhs/tile.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>

#include <stlw/debug.hpp>
#include <string>
#include <iostream>

namespace boomhs
{

TileType
tiletype_from_string(std::string const& string)
{
  return tiletype_from_string(string.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TilePosition
glm::vec3
operator+(TilePosition const& tp, glm::vec3 const& pos)
{
  auto const x = tp.x + pos.x;
  auto const y = 0.0f + pos.y;

  // The "TilePosition"'s y component gets translated to 3D in the z coordinate.
  auto const z = tp.y + pos.z;
  return glm::vec3{x, y, z};
}

std::ostream&
operator<<(std::ostream &stream, TilePosition const& tp)
{
  stream << "{";
  stream << std::to_string(tp.x);
  stream << ", ";
  stream << std::to_string(tp.y);
  stream << "}";
  return stream;
}

TilePosition
TilePosition::from_floats_truncated(float const x, float const y)
{
  auto const xx = static_cast<uint64_t>(x);
  auto const yy = static_cast<uint64_t>(y);
  return TilePosition{xx, yy};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Tile
bool
Tile::is_stair() const
{
  stlw::assert_exactly_only_one_true(is_stair_up(), is_stair_down());
  return is_stair_up() || is_stair_down();
}

bool
Tile::is_visible(EntityRegistry const& registry) const
{
  return registry.get<IsVisible>(this->eid).value;
}

void
Tile::set_isvisible(bool const v, EntityRegistry &registry)
{
  registry.get<IsVisible>(this->eid).value = v;
}

} // ns boomhs
