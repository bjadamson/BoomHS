#include <boomhs/state.hpp>
#include <boomhs/zone.hpp>

#include <imgui/imgui.hpp>

namespace boomhs
{

EngineState::EngineState(stlw::Logger &l, ImGuiIO &i, window::Dimensions const &d)
  : logger(l)
  , imgui(i)
  , dimensions(d)
  , player_collision(false)
  , mariolike_edges(false)
  , draw_entities(true)
  , draw_skybox(false)
  , draw_terrain(false)
  , draw_normals(false)
  , show_global_axis(true)
  , show_local_axis(false)
  , show_player_localspace_vectors(false)
  , show_player_worldspace_vectors(true)
{
}

GameState::GameState(EngineState &&es, ZoneManager &&zm)
  : engine_state(MOVE(es))
  , zone_manager(MOVE(zm))
{
}

} // ns boomhs
