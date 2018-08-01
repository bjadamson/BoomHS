#pragma once
#include <boomhs/zone_state.hpp>

#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <string>
#include <vector>

namespace boomhs
{
class EntityRegistry;

class LevelAssembler
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(LevelAssembler);

  static Result<ZoneStates, std::string>
  assemble_levels(stlw::Logger&, std::vector<EntityRegistry>&);
};

} // namespace boomhs
