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
  StairGenConfig const& stairconfig;
};

} // ns boomhs

namespace boomhs::level_generator
{

LevelData
make_leveldata(TileDataConfig const&, stlw::float_generator &, TileInfos const&,
    entt::DefaultRegistry &);

} // ns boomhs::level_generator
