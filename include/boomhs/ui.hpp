#pragma once
#include <imgui/imgui.hpp>
#include <boomhs/state.hpp>
#include <boomhs/entity.hpp>
#include <window/sdl_window.hpp>

namespace boomhs::detail
{

void
draw_entity_editor(GameState &state)
{
  auto &imgui = state.imgui;
  auto &eid_buffer = state.ui_state.eid_buffer;

  ImGui::InputInt("eid:", &eid_buffer, 0, state.entities.size());
  auto *const pe = state.entities[eid_buffer];
  assert(pe);
  auto &e = *pe;

  ImGui::InputFloat3("pos:", glm::value_ptr(e.translation));

  auto &euler_buffer = state.ui_state.euler_angle_buffer;
  euler_buffer = glm::degrees(glm::eulerAngles(e.rotation));

  if (ImGui::InputFloat3("rot:", glm::value_ptr(euler_buffer))) {
    e.rotation = glm::quat(euler_buffer * (3.14159f / 180.f));
  }

  ImGui::InputFloat3("scale:", glm::value_ptr(e.scale));
}

void
draw_camera_info(GameState &state)
{
  auto &camera = state.camera;

  ImGui::Begin("CAMERA INFO WINDOW");
  {
    auto const coords = camera.spherical_coordinates();
    auto const r = coords.radius_string();
    auto const t = coords.theta_string();
    auto const p = coords.phi_string();
    ImGui::Text("Spherical Coordinates:\nr: '%s', t: '%s', p: '%s'", r.c_str(), t.c_str(), p.c_str());
  }
  {
    auto const text = glm::to_string(camera.world_position());
    ImGui::Text("world position:\n'%s'", text.c_str());
  }
  {
    auto const coords = to_cartesian(camera.spherical_coordinates());
    auto const x = std::to_string(coords.x);
    auto const y = std::to_string(coords.y);
    auto const z = std::to_string(coords.z);
    ImGui::Text("Cartesian Coordinates (should match world position):\nx: '%s', y: '%s', z: '%s'",
        x.c_str(), y.c_str(), z.c_str());
  }
  {
    glm::vec3 const target_coords = camera.target_position();
    auto const x = std::to_string(target_coords.x);
    auto const y = std::to_string(target_coords.y);
    auto const z = std::to_string(target_coords.z);
    ImGui::Text("Follow Target position:\nx: '%s', y: '%s', z: '%s'",
        x.c_str(), y.c_str(), z.c_str());
  }
  ImGui::End();
}

void
draw_player_info(GameState &state)
{
  auto &player = state.player;

  ImGui::Begin("PLAYER INFO WINDOW");
  {
    auto const display = player.display();
    ImGui::Text("%s", display.c_str());


    glm::quat const quat = glm::angleAxis(glm::radians(0.0f), opengl::Y_UNIT_VECTOR);
    float const dot = glm::dot(player.orientation(), quat);
    std::string const dots = std::to_string(dot);
    ImGui::Text("dot product: '%s'", dots.c_str());
  }

  ImGui::End();
}

} // ns boomhs::detail

namespace boomhs
{

template<typename PROXY>
void draw_ui(GameState &state, window::SDLWindow &window, PROXY &proxy)
{
  using namespace detail;
  draw_entity_editor(state);
  draw_camera_info(state);
  draw_player_info(state);

  ImGui::Checkbox("Draw Skybox", &state.render.draw_skybox);

  if(ImGui::Checkbox("Enter Pressed", &state.ui_state.enter_pressed)) {
    int const& eid = state.ui_state.eid_buffer;;
    ecst::entity_id const id{static_cast<std::size_t>(eid)};
  }

  auto const window_menu = [&window, &state]() {
    if (ImGui::BeginMenu("Window")) {
      auto const draw_row = [&](char const* text, auto const fullscreen) {
        if (ImGui::MenuItem(text, nullptr, nullptr, state.window.fullscreen != fullscreen)) {
          window.set_fullscreen(fullscreen);
          state.window.fullscreen = fullscreen;
        }
      };
      draw_row("NOT Fullscreen", window::FullscreenFlags::NOT_FULLSCREEN);
      draw_row("Fullscreen", window::FullscreenFlags::FULLSCREEN);
      draw_row("Fullscreen DESKTOP", window::FullscreenFlags::FULLSCREEN_DESKTOP);
      ImGui::EndMenu();
    }
  };
  auto const camera_menu = [&state]() {
    if (ImGui::BeginMenu("Camera")) {
      ImGui::MenuItem("Flip Y", nullptr, &state.ui_state.flip_y);
      ImGui::EndMenu();
    }
  };
  auto const world_menu = [&state]() {
    if (ImGui::BeginMenu("World")) {
      ImGui::MenuItem("Global Axis", nullptr, &state.render.show_global_axis);
      ImGui::MenuItem("Local Axis", nullptr, &state.render.show_local_axis);
      ImGui::MenuItem("Target Forward/Right/Up Vectors", nullptr, &state.render.show_target_vectors);

      auto &tmap_render = state.render.tilemap;
      if (ImGui::MenuItem("Reveal Tilemap", nullptr, &tmap_render.reveal)) {
        tmap_render.redraw = true;
      }

      if (ImGui::BeginMenu("TileMap GridLines (Debug)")) {
        ImGui::MenuItem("Show (x, z)-axis lines", nullptr, &tmap_render.show_grid_lines);
        if (ImGui::MenuItem("Show y-axis Lines ", nullptr, &tmap_render.show_yaxis_lines)) {
          tmap_render.redraw = true;
        }
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
  };
  auto const player_menu = [&state]() {
    if (ImGui::BeginMenu("Player")) {
      ImGui::MenuItem("Player Collisions Enabled", nullptr, &state.collision.player);
      ImGui::EndMenu();
    }
  };

  if (ImGui::BeginMainMenuBar()) {
    window_menu();
    camera_menu();
    world_menu();
    player_menu();
  }
  ImGui::EndMainMenuBar();

  auto const& imgui = state.imgui;
  auto const framerate = imgui.Framerate;
  auto const ms_frame = 1000.0f / framerate;
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_frame, framerate);
}

} // ns boomhs
