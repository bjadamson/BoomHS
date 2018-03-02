#pragma once
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>
#include <ostream>
#include <string>
#include <type_traits>

namespace boomhs
{
class EntityRegistry;

enum class TileType : size_t
{
  FLOOR = 0,
  AT,
  BAR,
  BRIDGE,
  DOOR,
  RIVER,
  STAIR_DOWN,
  STAIR_UP,
  STAR,
  WALL,
  TELEPORTER,

  UNDEFINED
};

// We ensure the underlying type is size_t so we can use that assumption around the rest of the
// program. Very important this invariant does not change.
static_assert(std::is_same<size_t, std::underlying_type<TileType>::type>::value,
    "TileType must be size_t");

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
  CHECK("AT",         TileType::AT);
  CHECK("BAR",        TileType::BAR);
  CHECK("BRIDGE",     TileType::BRIDGE);
  CHECK("DOOR",       TileType::DOOR);
  CHECK("FLOOR",      TileType::FLOOR);
  CHECK("RIVER",      TileType::RIVER);
  CHECK("STAIR_DOWN", TileType::STAIR_DOWN);
  CHECK("STAIR_UP",   TileType::STAIR_UP);
  CHECK("STAR",       TileType::STAR);
  CHECK("TELEPORTER", TileType::TELEPORTER);
  CHECK("WALL",       TileType::WALL);

  CHECK("UNDEFINED",  TileType::UNDEFINED);
#undef CHECK
  // clang-format on

  // Logic error at this point
  std::abort();
  return TileType::UNDEFINED;
}

inline char const*
to_string(TileType const type)
{
#define CHECK(string, ttype)                                                                       \
  if (ttype == type) {                                                                             \
    return string;                                                                                 \
  }
  // clang-format off
  CHECK("AT",         TileType::AT);
  CHECK("BAR",        TileType::BAR);
  CHECK("BRIDGE",     TileType::BRIDGE);
  CHECK("DOOR",       TileType::DOOR);
  CHECK("FLOOR",      TileType::FLOOR);
  CHECK("RIVER",      TileType::RIVER);
  CHECK("STAR",       TileType::STAR);
  CHECK("STAIR_DOWN", TileType::STAIR_DOWN);
  CHECK("STAIR_UP",   TileType::STAIR_UP);
  CHECK("TELEPORT",   TileType::TELEPORTER);
  CHECK("WALL",       TileType::WALL);

  CHECK("UNDEFINED",  TileType::UNDEFINED);
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
    case TileType::AT:
      stream << "AT";
      break;
    case TileType::BRIDGE:
      stream << "BRIDGE";
      break;
    case TileType::FLOOR:
      stream << "FLOOR";
      break;
    case TileType::BAR:
      stream << "BAR";
      break;
    case TileType::DOOR:
      stream << "DOOR";
      break;
    case TileType::RIVER:
      stream << "RIVER";
      break;
    case TileType::STAR:
      stream << "STAR";
      break;
    case TileType::STAIR_DOWN:
      stream << "STAIR_DOWN";
      break;
    case TileType::STAIR_UP:
      stream << "STAIR_UP";
      break;
    case TileType::TELEPORTER:
      stream << "TELEPORTER";
      break;
    case TileType::WALL:
      stream << "WALL";
      break;
    case TileType::UNDEFINED:
      stream << "UNDEFINED";
      // fall-through to abort
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
  from_floats_truncated(float, float);

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
  TileType type = TileType::UNDEFINED;
  uint32_t eid;

  bool is_stair_up() const { return type == TileType::STAIR_UP; }
  bool is_stair_down() const { return type == TileType::STAIR_DOWN; }
  bool is_stair() const;

  bool is_visible(EntityRegistry const&) const;
  void set_isvisible(bool, EntityRegistry &);
};

inline bool
operator==(Tile const& a, Tile const& b)
{
  bool const same_eid = a.eid == b.eid;
  if (same_eid) {
    // debug sanity check
    assert(a.type == b.type);
  }
  return same_eid;
}

} // ns boomhs
