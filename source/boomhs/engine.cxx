#include <boomhs/engine.hpp>
#include <boomhs/controller.hpp>

#include <gl_sdl/sdl_window.hpp>

#include <extlibs/imgui.hpp>
#include <extlibs/openal.hpp>

using namespace gl_sdl;


namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// EngineState
EngineState::EngineState(common::Logger& l, ALCdevice& al, ImGuiIO& i, Frustum const& f)
    : logger(l)
    , al_device(al)
    , imgui(i)
    , frustum(f)
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
    , wireframe_override(false)
{
  behaviors.active = &behaviors.player_playing_behavior;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Engine
Engine::Engine(SDLWindow&& w, SDLControllers&& c)
    : window(MOVE(w))
    , controllers(MOVE(c))
{
  registries.resize(50);
}

Viewport
Engine::window_viewport() const
{
  return window.view_rect();
}

} // namespace boomhs
