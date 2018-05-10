#include <boomhs/level_manager.hpp>
#include <boomhs/state.hpp>
#include <window/controller.hpp>
#include <window/sdl_window.hpp>

#include <imgui/imgui.hpp>

using namespace window;

namespace boomhs
{

EngineState::EngineState(stlw::Logger& l, ImGuiIO& i, Dimensions const& d)
    : logger(l)
    , imgui(i)
    , dimensions(d)
    , player_collision(false)
    , mariolike_edges(false)
    , draw_imguimetrics(false)
    , draw_entities(true)
    , draw_fbo_testwindow(true)
    , draw_terrain(true)
    , draw_water(true)
    , draw_normals(false)
    , draw_sun(false)
    , show_global_axis(true)
    , show_local_axis(false)
    , show_player_localspace_vectors(false)
    , show_player_worldspace_vectors(true)
{
}

GameState::GameState(EngineState& es, LevelManager&& lm)
    : engine_state(es)
    , level_manager(MOVE(lm))
{
}

Engine::Engine(SDLWindow&& w, SDLControllers&& c)
    : window(MOVE(w))
    , controllers(MOVE(c))
{
  registries.resize(50);
}

} // namespace boomhs
