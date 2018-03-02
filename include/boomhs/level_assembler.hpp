#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/state.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <string>
#include <vector>

namespace opengl
{
} // ns opengl

namespace boomhs
{

class LevelAssembler
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelAssembler);

  static stlw::result<ZoneStates, std::string>
  assemble_levels(stlw::Logger &, std::vector<EntityRegistry> &);
};

} // ns boomhs
