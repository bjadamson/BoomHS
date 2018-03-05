#pragma once
#include <stlw/type_macros.hpp>

namespace stlw
{
class float_generator;
} // ns stlw

namespace boomhs
{
class EntityRegistry;
class TileGrid;
struct TilePosition;

enum class Alignment
{
  EVIL = 0,
  NEUTRAL,
  GOOD,
  NOT_SET
};

char const*
alignment_to_string(Alignment const);

struct NPCData
{
  char const* name = "NOT SET. ERROR";
  int health = -1;
  int level = -1;
  Alignment alignment = Alignment::NOT_SET;
};

class NPC
{
  NPC() = delete;
public:

  // Loads a new NPC into the EntityRegistry.
  static void
  create(EntityRegistry &, char const*, TilePosition const&);

  static void
  create_random(TileGrid const&, EntityRegistry &, stlw::float_generator &);
};

} // ns boomhs
