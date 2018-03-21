#include <boomhs/state.hpp>
#include <boomhs/level_manager.hpp>

#include <imgui/imgui.hpp>

namespace boomhs
{

EngineState::EngineState(stlw::Logger &l, ImGuiIO &i, window::Dimensions const &d)
  : logger(l)
  , imgui(i)
  , dimensions(d)
  , player_collision(false)
  , mariolike_edges(false)
  , draw_imguimetrics(false)
  , draw_entities(true)
  , draw_terrain(false)
  , draw_normals(false)
  , show_global_axis(true)
  , show_local_axis(false)
  , show_player_localspace_vectors(false)
  , show_player_worldspace_vectors(true)
{
}

GameState::GameState(EngineState &&es, LevelManager &&lm)
  : engine_state(MOVE(es))
  , level_manager(MOVE(lm))
{
}

} // ns boomhs
