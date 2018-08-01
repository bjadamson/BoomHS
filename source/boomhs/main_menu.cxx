#include <boomhs/audio.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/state.hpp>
#include <opengl/colors.hpp>
#include <window/sdl_window.hpp>

#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

// clang-format off
auto static constexpr WINDOW_FLAGS = (0
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    );
// clang-format on

auto static constexpr STYLE_VARS = (0 | ImGuiStyleVar_ChildRounding);

void
draw_menu(EngineState& es, ImVec2 const& size, WaterAudioSystem& water_audio)
{
  bool const draw_debug = es.ui_state.draw_debug_ui;
  auto&      main_menu  = es.main_menu;

  auto const fn = [&]() {
    {
      constexpr char const* resume = "Resume Game";
      constexpr char const* start  = "Start Game";

      bool&       game_running = es.game_running;
      char const* text         = game_running ? resume : start;

      bool const button_pressed = ImGui::Button(text);
      main_menu.show            = !button_pressed;

      if (button_pressed) {
        game_running = true;
      }
    }
    main_menu.show_options |= ImGui::Button("Options");
    if (main_menu.show_options) {
      auto const fn = [&]() {
        ImGui::Text("Options");
        ImGui::Separator();
        ImGui::Separator();
        ImGui::Text("Audio");

        auto& uistate = es.ui_state;
        auto& ui_debug = uistate.debug;
        auto& audio   = ui_debug.buffers.audio;
        if (ImGui::SliderFloat("Ambient Volume", &audio.ambient, 0.0f, 1.0f)) {
          water_audio.set_volume(audio.ambient);
        }
        ImGui::Separator();
        ImGui::Text("Gameplay");
        ImGui::Separator();
        ImGui::Text("Graphics");
        auto& gs = es.graphics_settings;
        {
          auto constexpr GRAPHICS_MODE = stlw::make_array<GameGraphicsMode>(
              GameGraphicsMode::Basic, GameGraphicsMode::Medium, GameGraphicsMode::Advanced);

          gs.mode = imgui_cxx::combo_from_array("Graphics Settings", "Basic\0Medium\0Advanced\0\0",
                                                &ui_debug.buffers.water.selected_water_graphicsmode,
                                                GRAPHICS_MODE);
        }
        ImGui::Separator();
        ImGui::Checkbox("Disable Sun Shafts", &gs.disable_sunshafts);

        ImGui::Separator();
        ImGui::Text("IMGUI");
        ImGui::Checkbox("Draw Debug", &uistate.draw_debug_ui);
        ImGui::Checkbox("Draw InGame", &uistate.draw_ingame_ui);
      };
      imgui_cxx::with_window(fn, "Options Window");
    }
    es.quit |= ImGui::Button("Exit");
  };
  auto const draw_window = [&]() {
    auto const y_offset   = draw_debug ? imgui_cxx::main_menu_bar_size().y : 0;
    auto const window_pos = ImVec2(0, y_offset);
    ImGui::SetNextWindowPos(window_pos);

    ImGui::SetNextWindowSize(size);
    imgui_cxx::with_window(fn, "Main Menu", nullptr, WINDOW_FLAGS);
  };
  imgui_cxx::with_stylevars(draw_window, STYLE_VARS, 5.0f);
}

void
process_keydown(GameState& state, SDL_Event const& event)
{
  auto& es = state.engine_state;
  auto& ui = es.ui_state;

  switch (event.key.keysym.sym) {
  case SDLK_ESCAPE:
    if (es.game_running) {
      es.main_menu.show ^= true;
    }
    break;
  case SDLK_F10:
    es.quit = true;
    break;
  case SDLK_F11:
    ui.draw_debug_ui ^= true;
    break;
  }
}

} // namespace

namespace boomhs::main_menu
{

void
draw(EngineState& es, ImVec2 const& size, WaterAudioSystem& water_audio)
{
  draw_menu(es, size, water_audio);
}

void
process_event(GameState& state, SDL_Event& event, Camera& camera, FrameTime const& ft)
{
  switch (event.type) {
  case SDL_KEYDOWN:
    process_keydown(state, event);
    break;
  }
}

} // namespace boomhs::main_menu
