#include <boomhs/engine.hpp>

#include <window/controller.hpp>
#include <window/sdl_window.hpp>

#include <extlibs/imgui.hpp>
#include <extlibs/openal.hpp>

using namespace window;


namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// EngineState
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
    , draw_view_frustum(false)
    , draw_2d_billboard_entities(true)
    , draw_2d_ui_entities(true)
    , draw_3d_entities(true)
    , draw_fbo_testwindow(true)
    , draw_terrain(true)
    , draw_normals(false)
    , draw_skybox(true)
    , show_global_axis(false)
    , show_player_localspace_vectors(false)
    , show_player_worldspace_vectors(false)
    , show_grid_lines(false)
    , show_yaxis_lines(false)
    , wireframe_override(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Engine
Engine::Engine(SDLWindow&& w, SDLControllers&& c)
    : window(MOVE(w))
    , controllers(MOVE(c))
{
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
