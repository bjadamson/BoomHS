#pragma once
#include <boomhs/components.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/stairwell_generator.hpp>
#include <stlw/log.hpp>
#include <stlw/random.hpp>

namespace opengl
{
class TextureTable;
class ShaderPrograms;
} // namespace opengl

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

} // namespace boomhs

namespace boomhs::dungeon_generator
{

LevelGeneratedData
gen_level(stlw::Logger&, LevelConfig const&, EntityRegistry&, stlw::float_generator&,
          opengl::ShaderPrograms&, opengl::TextureTable&);

} // namespace boomhs::dungeon_generator
