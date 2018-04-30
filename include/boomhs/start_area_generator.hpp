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

struct StartAreaGenerator
{
  static LevelGeneratedData
  gen_level(stlw::Logger&, EntityRegistry&, stlw::float_generator&, opengl::TextureTable const&);

  StartAreaGenerator() = delete;
};

} // namespace boomhs
