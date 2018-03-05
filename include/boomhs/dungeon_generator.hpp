#pragma once
#include <stlw/random.hpp>
#include <boomhs/components.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <boomhs/leveldata.hpp>

namespace opengl
{
class TextureTable;
} // ns opengl

namespace boomhs
{
class EntityRegistry;

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

namespace boomhs::dungeon_generator
{

LevelGeneredData
gen_level(LevelConfig const&, EntityRegistry &, stlw::float_generator &,
    opengl::TextureTable const&);

} // ns boomhs::dungeon_generator
