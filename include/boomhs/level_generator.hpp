#pragma once
#include <stlw/random.hpp>
#include <entt/entt.hpp>
#include <boomhs/components.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/tilemap.hpp>

namespace boomhs
{

struct TilemapConfig
{
  uint32_t const width, length;
  StairGenConfig const& stairconfig;
};

} // ns boomhs

namespace boomhs::level_generator
{

std::pair<TileMap, TilePosition>
make_tilemap(TilemapConfig &, stlw::float_generator &, entt::DefaultRegistry &);

} // ns boomhs::level_generator
