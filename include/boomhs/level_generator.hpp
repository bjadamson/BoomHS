#pragma once
#include <stlw/random.hpp>
#include <boomhs/components.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>

namespace boomhs
{
class EntityRegistry;
struct TileSharedInfoTable;

struct TileGridConfig
{
  size_t const width, height;
};

struct LevelConfig
{
  StairGenConfig const& stairconfig;
  TileGridConfig const& tileconfig;
};

} // ns boomhs

namespace boomhs::level_generator
{

LevelData
make_leveldata(LevelConfig const&, EntityRegistry &, TileSharedInfoTable &&,
    stlw::float_generator &);

} // ns boomhs::level_generator
