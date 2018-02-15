#pragma once
#include <stlw/random.hpp>
#include <entt/entt.hpp>
#include <boomhs/components.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>

namespace boomhs
{
struct TileInfos;

struct TileDataConfig
{
  size_t const width, height;
};

struct LevelConfig
{
  StairGenConfig const& stairconfig;
  TileDataConfig const& tileconfig;
};

} // ns boomhs

namespace boomhs::level_generator
{

LevelData
make_leveldata(LevelConfig const&, entt::DefaultRegistry &, TileInfos &&,
    stlw::float_generator &);

} // ns boomhs::level_generator
