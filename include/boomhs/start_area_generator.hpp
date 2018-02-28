#pragma once
#include <boomhs/leveldata.hpp>

namespace boomhs
{

class EntityRegistry;

struct StartAreaGenerator
{
  static LevelGeneredData
  gen_level(EntityRegistry &, stlw::float_generator &);

  StartAreaGenerator() = delete;
};

} // ns boomhs
