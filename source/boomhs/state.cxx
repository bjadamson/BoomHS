#include <boomhs/level_manager.hpp>
#include <boomhs/state.hpp>
#include <window/controller.hpp>
#include <window/sdl_window.hpp>

#include <imgui/imgui.hpp>

using namespace window;

namespace boomhs
{

EngineState::EngineState(common::Logger& l, ALCdevice& al, ImGuiIO& i, ScreenDimensions const& d)
    : logger(l)
    , al_device(al)
    , imgui(i)
    , dimensions(d)
    , disable_controller_input(true)
    , player_collision(false)
    , mariolike_edges(false)
    , draw_imguimetrics(false)
    , draw_bounding_boxes(false)
    , draw_2d_billboard_entities(true)
    , draw_2d_ui_entities(true)
    , draw_3d_entities(true)
    , draw_fbo_testwindow(true)
    , draw_terrain(true)
    , draw_normals(false)
    , draw_skybox(true)
    , show_global_axis(false)
    , show_local_axis(false)
    , show_player_localspace_vectors(false)
    , show_player_worldspace_vectors(false)
    , show_grid_lines(false)
    , show_yaxis_lines(false)
    , wireframe_override(false)
{
}

GameState::GameState(EngineState& es, LevelManager&& lm)
    : engine_state(es)
    , level_manager(MOVE(lm))
{
  auto& logger = engine_state.logger;
  LOG_ERROR("GameState Initialized");
}

GameState::GameState(GameState&& o)
    : engine_state(o.engine_state)
    , level_manager(MOVE(o.level_manager))
{
  auto& logger = engine_state.logger;
  LOG_ERROR("GameState MOVED");
}

Engine::Engine(SDLWindow&& w, SDLControllers&& c)
    : window(MOVE(w))
    , controllers(MOVE(c))
{
  std::cerr << "Engine constructed\n";
  registries.resize(50);
}

ScreenDimensions
Engine::dimensions() const
{
  return window.get_dimensions();
}

ScreenSize
Engine::screen_size() const
{
  auto const d = dimensions();
  return ScreenSize{d.right(), d.bottom()};
}

} // namespace boomhs
