#include <boomhs/main_menu.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <opengl/colors.hpp>
#include <window/sdl_window.hpp>

#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

auto static constexpr WINDOW_FLAGS = (0
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    );

auto static constexpr STYLE_VARS = (0
    | ImGuiStyleVar_ChildRounding);

void
draw_menu(EngineState& es, ImVec2 const& size)
{
  bool const draw_debug = es.ui_state.draw_debug_ui;
  auto const fn = [&]() {
    {
      constexpr char const* resume = "Resume Game";
      constexpr char const* start = "Start Game";

      bool &game_running = es.game_running;
      char const* text = game_running ? resume : start;

      bool const button_pressed = ImGui::Button(text);
      es.show_main_menu = !button_pressed;


      if (button_pressed) {
        game_running = true;
      }
    }
    ImGui::Button("Options");
    es.quit = ImGui::Button("Exit");
  };
  auto const draw_window = [&]() {

    auto const y_offset = draw_debug
      ? imgui_cxx::main_menu_bar_size().y
      : 0;
    auto const window_pos = ImVec2(0, y_offset);
    ImGui::SetNextWindowPos(window_pos);

    ImGui::SetNextWindowSize(size);
    imgui_cxx::with_window(fn, "Main Menu", nullptr, WINDOW_FLAGS);
  };
  imgui_cxx::with_stylevar(draw_window, STYLE_VARS, 5.0f);

}

} // ns anon

namespace boomhs::main_menu
{

void
draw(EngineState& es, ImVec2 const& size)
{
  render::clear_screen(LOC::BLACK);
  draw_menu(es, size);
}

bool
process_event(GameState& state, SDL_Event &event, FrameTime const& ft)
{
  auto& es = state.engine_state;

  switch (event.key.keysym.sym)
  {
  case SDLK_ESCAPE:
    if (es.game_running) {
      es.show_main_menu ^= true;
    }
    break;
  }
  return window::is_quit_event(event);
}

} // ns boomhs::main_menu
