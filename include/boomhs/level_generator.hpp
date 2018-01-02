#pragma once
#include <stlw/random.hpp>
#include <boomhs/tilemap.hpp>

namespace boomhs::level_generator
{

std::pair<TileMap, TilePosition>
make_tilemap(int const width, int const height, int const length, stlw::float_generator &);

} // ns boomhs::level_generator
