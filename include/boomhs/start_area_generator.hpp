#pragma once
#include <boomhs/leveldata.hpp>

namespace opengl
{
class TextureTable;
} // ns opengl

namespace boomhs
{

class EntityRegistry;

struct StartAreaGenerator
{
  static LevelGeneredData
  gen_level(EntityRegistry &, stlw::float_generator &, opengl::TextureTable const&);

  StartAreaGenerator() = delete;
};

} // ns boomhs
