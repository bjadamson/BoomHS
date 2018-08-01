#pragma once
#include <boomhs/leveldata.hpp>
#include <stlw/log.hpp>

namespace opengl
{
class TextureTable;
} // namespace opengl

namespace boomhs
{
class EntityRegistry;
class Heightmap;

struct StartAreaGenerator
{
  static LevelGeneratedData
  gen_level(stlw::Logger&, EntityRegistry&, stlw::float_generator&, opengl::ShaderPrograms&,
            opengl::TextureTable&, MaterialTable const&, Heightmap const&);

  StartAreaGenerator() = delete;
};

} // namespace boomhs
