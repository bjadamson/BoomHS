#include <boomhs/audio.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/colors.hpp>
#include <boomhs/components.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/state.hpp>
#include <boomhs/zone_state.hpp>

#include <opengl/renderer.hpp>
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
      auto& uistate = es.ui_state;
      auto const fn = [&]() {
        ImGui::Text("Options");
        ImGui::Checkbox("Disable Controller Input", &es.disable_controller_input);
        ImGui::Text("UI");
        ImGui::Checkbox("Draw Debug", &uistate.draw_debug_ui);
        ImGui::Checkbox("Draw InGame", &uistate.draw_ingame_ui);
        ImGui::Separator();
        ImGui::Text("Audio");

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
          auto constexpr GRAPHICS_MODE = common::make_array<GameGraphicsMode>(
              GameGraphicsMode::Basic, GameGraphicsMode::Medium, GameGraphicsMode::Advanced);

          gs.mode = imgui_cxx::combo_from_array("Graphics Settings", "Basic\0Medium\0Advanced\0\0",
                                                &ui_debug.buffers.water.selected_water_graphicsmode,
                                                GRAPHICS_MODE);
        }
        ImGui::Separator();
        ImGui::Checkbox("Disable Sun Shafts", &gs.disable_sunshafts);
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
draw_debugwindow(EngineState& es, ZoneState& zs)
{
  auto& registry = zs.registry;
  ImGui::Checkbox("Draw Skybox", &es.draw_skybox);
  {
    auto const eids = find_orbital_bodies(registry);
    auto       num  = 1;
    for (auto const eid : eids) {
      auto& hidden    = registry.get<IsRenderable>(eid).hidden;

      auto const text = "Draw Orbital Body" + std::to_string(num++);
      ImGui::Checkbox(text.c_str(), &hidden);
    }
  }

  auto& uistate = es.ui_state;
  auto& debugstate = uistate.debug;
  ImGui::Checkbox("Draw 3D Entities", &es.draw_3d_entities);
  ImGui::Checkbox("Draw 2D Billboard Entities", &es.draw_2d_billboard_entities);
  ImGui::Checkbox("Draw 2D UI Entities", &es.draw_2d_ui_entities);
  ImGui::Checkbox("Draw Terrain", &es.draw_terrain);

  {
    auto& water_buffer = es.ui_state.debug.buffers.water;
    ImGui::Checkbox("Draw Water", &water_buffer.draw);
  }

  ImGui::Checkbox("Draw Bounding Boxes", &es.draw_bounding_boxes);
  ImGui::Checkbox("Draw Normals", &es.draw_normals);
  ImGui::Checkbox("View View Frustum", &es.draw_view_frustum);
  ImGui::Checkbox("Draw Wireframe Rendering", &es.wireframe_override);

  ImGui::Separator();
  ImGui::Text("IMGUI");
  ImGui::Checkbox("Draw Debug", &uistate.draw_debug_ui);
  ImGui::Checkbox("Draw InGame", &uistate.draw_ingame_ui);

  ImGui::Separator();
  ImGui::Checkbox("Mariolike Edges", &es.mariolike_edges);

  ImGui::Checkbox("Show (x, z)-axis lines", &es.show_grid_lines);
  ImGui::Checkbox("Show y-axis Lines ", &es.show_yaxis_lines);

  ImGui::Separator();
  ImGui::Checkbox("ImGui Metrics", &es.draw_imguimetrics);
  if (es.draw_imguimetrics) {
    ImGui::ShowMetricsWindow(&es.draw_imguimetrics);
  }
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

void
show_ambientlight_window(UiDebugState& ui, LevelData& ldata)
{
  auto const draw = [&]() {
    ImGui::Text("Global Light");

    auto& global_light = ldata.global_light;
    ImGui::ColorEdit3("Ambient Light Color:", global_light.ambient.data());

    if (ImGui::Button("Close", ImVec2(120, 0))) {
      ui.show_ambientlight_window = false;
    }
  };
  imgui_cxx::with_window(draw, "Global Light Editor");
}

void
show_directionallight_window(UiDebugState& ui, LevelData& ldata)
{
  auto& directional = ldata.global_light.directional;

  auto const draw = [&]() {
    ImGui::Text("Directional Light");
    ImGui::InputFloat3("direction:", glm::value_ptr(directional.direction));

    auto& colors = directional.light;
    ImGui::ColorEdit3("Diffuse:", colors.diffuse.data());
    ImGui::ColorEdit3("Specular:", colors.specular.data());

    if (ImGui::Button("Close", ImVec2(120, 0))) {
      ui.show_directionallight_window = false;
    }
  };
  imgui_cxx::with_window(draw, "Directional Light Editor");
}

void
lighting_menu(EngineState& es, LevelData& ldata, EntityRegistry& registry)
{
  auto& ui                     = es.ui_state.debug;
  bool& edit_ambientlight      = ui.show_ambientlight_window;
  bool& edit_directionallights = ui.show_directionallight_window;

  auto const draw = [&]() {
    ImGui::MenuItem("Ambient Lighting", nullptr, &edit_ambientlight);
    ImGui::MenuItem("Directional Lighting", nullptr, &edit_directionallights);
  };
  imgui_cxx::with_menu(draw, "Lightning");
  if (edit_ambientlight) {
    show_ambientlight_window(ui, ldata);
  }
  if (edit_directionallights) {
    show_directionallight_window(ui, ldata);
  }
}

void
world_menu(EngineState& es, LevelData& ldata)
{
  auto&      ui   = es.ui_state.debug;
  auto const draw = [&]() {
    ImGui::MenuItem("Update Orbital", nullptr, &es.update_orbital_bodies);
    ImGui::MenuItem("Draw Global Axis", nullptr, &es.show_global_axis);
  };
  imgui_cxx::with_menu(draw, "World");
}

void
draw_mainmenu(EngineState& es, LevelManager& lm, window::SDLWindow& window, DrawState& ds)
{
  auto&      uistate      = es.ui_state.debug;
  auto const windows_menu = [&]() {
    ImGui::MenuItem("Camera", nullptr, &uistate.show_camerawindow);
    ImGui::MenuItem("Entity", nullptr, &uistate.show_entitywindow);
    ImGui::MenuItem("Device", nullptr, &uistate.show_devicewindow);
    ImGui::MenuItem("Environment Window", nullptr, &uistate.show_environment_window);
    ImGui::MenuItem("Player", nullptr, &uistate.show_playerwindow);
    ImGui::MenuItem("Skybox", nullptr, &uistate.show_skyboxwindow);
    ImGui::MenuItem("Terrain", nullptr, &uistate.show_terrain_editor_window);
    ImGui::MenuItem("Time", nullptr, &uistate.show_time_window);
    ImGui::MenuItem("Water", nullptr, &uistate.show_water_window);
    ImGui::MenuItem("Exit", nullptr, &es.quit);
  };

  auto&      window_state  = es.window_state;
  auto const settings_menu = [&]() {
    auto const setwindow_row = [&](char const* text, auto const fullscreen) {
      if (ImGui::MenuItem(text, nullptr, nullptr, window_state.fullscreen != fullscreen)) {
        window.set_fullscreen(fullscreen);
        window_state.fullscreen = fullscreen;
      }
    };
    setwindow_row("NOT Fullscreen", window::FullscreenFlags::NOT_FULLSCREEN);
    setwindow_row("Fullscreen", window::FullscreenFlags::FULLSCREEN);
    setwindow_row("Fullscreen DESKTOP", window::FullscreenFlags::FULLSCREEN_DESKTOP);
    auto const setsync_row = [&](char const* text, auto const sync) {
      if (ImGui::MenuItem(text, nullptr, nullptr, window_state.sync != sync)) {
        window.set_swapinterval(sync);
        window_state.sync = sync;
      }
    };
    setsync_row("Synchronized", window::SwapIntervalFlag::SYNCHRONIZED);
    setsync_row("Late Tearing", window::SwapIntervalFlag::LATE_TEARING);
  };

  auto&      zs            = lm.active();
  auto&      ldata         = zs.level_data;
  auto&      registry      = zs.registry;
  auto const draw_mainmenu = [&]() {
    imgui_cxx::with_menu(windows_menu, "Windows");
    imgui_cxx::with_menu(settings_menu, "Settings");
    world_menu(es, ldata);
    lighting_menu(es, ldata, registry);

    auto const framerate = es.imgui.Framerate;
    auto const ms_frame  = 1000.0f / framerate;

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.30f);
    ImGui::Text("#verts: %s", ds.to_string().c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.60f);
    ImGui::Text("Current Level: %i", lm.active_zone());

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.76f);
    ImGui::Text("FPS(avg): %.1f ms/frame: %.3f", framerate, ms_frame);
  };
  imgui_cxx::with_mainmenubar(draw_mainmenu);
}

} // namespace

namespace boomhs::main_menu
{

void
draw(EngineState& es, SDLWindow& window, DrawState& ds, LevelManager& lm, ImVec2 const& size,
     WaterAudioSystem& water_audio)
{
  draw_menu(es, size, water_audio);

  auto& uistate = es.ui_state.debug;
  if (uistate.show_debugwindow) {
    auto& zs = lm.active();
    draw_debugwindow(es, zs);
  }

  draw_mainmenu(es, lm, window, ds);
}

void
process_event(SDLEventProcessArgs && epa)
{
  auto& state    = epa.game_state;
  auto& event    = epa.event;
  auto& camera   = epa.camera;
  auto const& ft = epa.frame_time;

  switch (event.type) {
  case SDL_KEYDOWN:
    process_keydown(state, event);
    break;
  }
}

} // namespace boomhs::main_menu
