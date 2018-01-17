#include <boomhs/ui.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui/imgui.hpp>
#include <boomhs/state.hpp>
#include <stlw/format.hpp>
#include <window/sdl_window.hpp>
#include <iostream>

namespace
{
using namespace boomhs;

void
draw_entity_editor(GameState &state)
{
  auto &imgui = state.imgui;
  auto &eid_buffer = state.ui_state.eid_buffer;

  auto constexpr ENTITIES_S =
    "COLOR_CUBE\0"
    "TEXTURE_CUBE\0"
    "WIREFRAME_CUBE\0"
    "SKYBOX\0"
    "HOUSE\0"
    "AT\0"
    "PLAYER\0"
    "TILEMAP\0"
    "TERRAIN\0"
    "CAMERA\0"
    "GLOBAL_X\0"
    "GLOBAL_Y\0"
    "GLOBAL_Z\0"
    "LOCAL_X\0"
    "LOCAL_Y\0"
    "LOCAL_Z\0"

    "LOCAL_FORWARD\0"
    "CAMERA_ARROW_0\0"
    "CAMERA_ARROW_1\0"
    "CAMERA_ARROW_2\0"

    "ORC\0"
    "TROLL\0"

    "LIGHT\0"
    "\0";

  auto &current = state.ui_state.entity_window_current;
  if (ImGui::Combo("Entity", &current, ENTITIES_S)) {
    auto &entity = *state.entities[current];
    state.camera.set_target(entity);
    state.player.set_transform(entity);
  }

  auto &entity = *state.entities[current];
  ImGui::InputFloat3("pos:", glm::value_ptr(entity.translation));
  {
    auto buffer = glm::degrees(glm::eulerAngles(entity.rotation));
    if (ImGui::InputFloat3("rot:", glm::value_ptr(buffer))) {
      entity.rotation = glm::quat(buffer * (3.14159f / 180.f));
    }
  }
  ImGui::InputFloat3("scale:", glm::value_ptr(entity.scale));
}

void
draw_camera_info(GameState &state)
{
  auto &camera = state.camera;
  auto &player = state.player;

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
  ImGui::Separator();
  ImGui::Separator();
  {
    auto const& player_transform = player.transform();
    auto const scoords = to_spherical(player_transform.translation);
    auto const ccoords = camera.spherical_coordinates();

    //glm::mat4 const origin_m = glm::translate(glm::mat4{}, glm::vec3{0.0f});
    //glm::mat4 m = origin_m * glm::toMat4(player.orientation());
    //glm::vec3 player_fwd = origin_m * glm::vec4{player.forward_vector(), 1.0f};
    //player_fwd = glm::normalize(player_fwd);

    //glm::vec3 const camera_fwd = camera.forward_vector();
    //float const theta = acos(glm::dot(player_fwd, camera_fwd));
    ImGui::Text("player xyz: '%s', player phi: '%s' player theta: '%s'\n",
        glm::to_string(player_transform.translation).c_str(),
        std::to_string(scoords.phi).c_str(),
        std::to_string(scoords.theta).c_str()
        );
    ImGui::Separator();
    ImGui::Text("camera xyz: '%s', camera phi: '%s', camera theta: '%s'\n",
        glm::to_string(camera.world_position()).c_str(),
        std::to_string(camera.spherical_coordinates().phi).c_str(),
        std::to_string(camera.spherical_coordinates().theta).c_str()
        );
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

void
show_lighting_window(GameState &state)
{
  if (ImGui::Begin("Lighting")) {
    auto const floatslider = [](char const* text, auto &color, auto const& fn) {
      auto array = color.to_array();
      if (fn(text, array.data())) {
        color = opengl::Color{array};
      }
    };
    auto const color_float3slider = [&floatslider](char const* text, auto &color) {
      auto array = color.to_array();
      if (ImGui::ColorEdit3(text, array.data())) {
        color = opengl::Color{array};
      }
    };
    auto const color_float4slider = [](char const* text, auto &color) {
      auto array = color.to_array();
      if (ImGui::ColorEdit4(text, array.data())) {
        color = opengl::Color{array};
      }
    };

    ImGui::Separator();
    ImGui::Separator();
    color_float4slider("Background Color", state.render.background);

    ImGui::Text("@/+/# (Player) Material");
    ImGui::Separator();
    ImGui::Separator();

    color_float4slider("@/+/# Ambient", state.at_materials.ambient);
    color_float4slider("@/+/# Diffuse", state.at_materials.diffuse);
    color_float4slider("@/+/# Specular", state.at_materials.specular);
    ImGui::SliderFloat("@/+/# Shininess", &state.at_materials.shininess, 0.0f, 128.0f);

    ImGui::Text("Light Instance #0");
    ImGui::Separator();
    ImGui::Separator();

    {
      auto &t = state.light.single_light_position;
      auto *light_pos = glm::value_ptr(t);
      ImGui::SliderFloat3("Light Position", light_pos, -100.0f, 100.0f);
      std::string const s = glm::to_string(glm::normalize(t));
      ImGui::Text("Light Direction: '%s'", s.c_str());
    }
    color_float4slider("Light Ambient", state.light.ambient);
    color_float4slider("Light Diffuse", state.light.diffuse);
    color_float4slider("Light Specular", state.light.specular);

    auto &current_item = state.ui_state.attenuation_current_item;
    if (ImGui::Combo("Attenuation", &current_item, opengl::ATTENUATION_DISTANCE_STRINGS)) {
      state.light.attenuation = opengl::ATTENUATION_VALUE_TABLE[current_item];
    }

    if (ImGui::Button("Close", ImVec2(120,0))) {
      state.ui_state.show_lighting_window = false;
    }
    ImGui::End();
  }
}

void
world_menu(GameState &state)
{
  bool &edit_lighting = state.ui_state.show_lighting_window;
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
    ImGui::MenuItem("Lighting", nullptr, &edit_lighting);
    ImGui::EndMenu();
  }
  if (edit_lighting) {
    show_lighting_window(state);
  }
}

} // ns anonymous

namespace boomhs
{

void
draw_ui(GameState &state, window::SDLWindow &window)
{
  draw_entity_editor(state);
  draw_camera_info(state);
  draw_player_info(state);

  ImGui::Checkbox("Draw Skybox", &state.render.draw_skybox);
  ImGui::Checkbox("Enter Pressed", &state.ui_state.enter_pressed);
  ImGui::Checkbox("Mouse Rotation Lock", &state.ui_state.rotate_lock);

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
  auto const player_menu = [&state]() {
    if (ImGui::BeginMenu("Player")) {
      ImGui::MenuItem("Player Collisions Enabled", nullptr, &state.collision.player);
      ImGui::EndMenu();
    }
  };

  if (ImGui::BeginMainMenuBar()) {
    window_menu();
    camera_menu();
    world_menu(state);
    player_menu();
  }
  ImGui::EndMainMenuBar();

  auto const& imgui = state.imgui;
  auto const framerate = imgui.Framerate;
  auto const ms_frame = 1000.0f / framerate;
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_frame, framerate);
}

} // ns anonymous
