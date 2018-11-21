#pragma once
#include <boomhs/leveldata.hpp>
#include <common/log.hpp>

namespace opengl
{
class TextureTable;
} // namespace opengl

namespace boomhs
{
class  EntityRegistry;
class  Heightmap;
struct WorldOrientation;

struct StartAreaGenerator
{
  static LevelGeneratedData
  gen_level(common::Logger&, EntityRegistry&, RNG&, opengl::ShaderPrograms&, opengl::TextureTable&,
            MaterialTable const&, Heightmap const&, WorldOrientation const&);

  StartAreaGenerator() = delete;
};

} // namespace boomhs
