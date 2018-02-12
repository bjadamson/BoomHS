#pragma once
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>
#include <ostream>
#include <string>

namespace boomhs
{

enum class TileType : size_t
{
  FLOOR = 0,
  WALL,
  RIVER,
  BRIDGE,
  STAIR_DOWN,
  STAIR_UP,
  MAX
};

TileType
tiletype_from_string(std::string const&);

// No point defining in cxx file, need to update every time we add a new tile anyways.
inline TileType
tiletype_from_string(char const* cstring)
{
#define CHECK(string, type)                                                                        \
  if (0 == ::strcmp(string, cstring)) {                                                            \
    return type;                                                                                   \
  }

  // clang-format off
  CHECK("FLOOR",      TileType::FLOOR);
  CHECK("WALL",       TileType::WALL);
  CHECK("RIVER",      TileType::RIVER);
  CHECK("BRIDGE",     TileType::BRIDGE);
  CHECK("STAIR_DOWN", TileType::STAIR_DOWN);
  CHECK("STAIR_UP",   TileType::STAIR_UP);
#undef CHECK
  // clang-format on

  // Logic error at this point
  std::abort();
  return TileType::MAX;
}

inline char const*
to_string(TileType const type)
{
#define CHECK(string, ttype)                                                                       \
  if (ttype == type) {                                                                             \
    return string;                                                                                 \
  }
  // clang-format off
  CHECK("FLOOR",      TileType::FLOOR);
  CHECK("WALL",       TileType::WALL);
  CHECK("RIVER",      TileType::RIVER);
  CHECK("BRIDGE",     TileType::BRIDGE);
  CHECK("STAIR_DOWN", TileType::STAIR_DOWN);
  CHECK("STAIR_UP",   TileType::STAIR_UP);
#undef CHECK
  // clang-format on
  // Logic error at this point
  std::abort();
  return nullptr;
}

// No point defining in cxx file, need to update every time we add a new tile anyways.
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
