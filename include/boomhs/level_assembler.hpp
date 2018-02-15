#pragma once

#include <boomhs/state.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace boomhs
{

class LevelAssembler
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelAssembler);

  static stlw::result<ZoneStates, std::string>
  assemble_levels(stlw::Logger &, std::vector<entt::DefaultRegistry> &);
};

} // ns boomhs
