#pragma once
#include <stlw/type_macros.hpp>

namespace boomhs
{
class EntityRegistry;
struct TilePosition;

struct EnemyData
{
};

class Enemy
{
  Enemy() = delete;
public:

  // Loads a new Enemy into the EntityRegistry.
  static void
  load_new(EntityRegistry &, char const*, TilePosition const&);
};

} // ns boomhs
