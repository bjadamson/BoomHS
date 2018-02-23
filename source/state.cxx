#include <boomhs/state.hpp>
#include <boomhs/zone.hpp>

#include <imgui/imgui.hpp>

namespace boomhs
{

EngineState::EngineState(stlw::Logger &l, ImGuiIO &i, window::Dimensions const &d)
  : logger(l)
  , imgui(i)
  , dimensions(d)
{
}

GameState::GameState(EngineState &&es, ZoneManager &&zm)
  : engine_state(MOVE(es))
  , zone_manager(MOVE(zm))
{
}

} // ns boomhs
