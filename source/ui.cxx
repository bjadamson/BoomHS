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
using namespace opengl;

void
draw_entity_editor(GameState &state, entt::DefaultRegistry &registry)
{
  using pair_t = std::pair<std::string, std::uint32_t>;
  auto const combo_callback = [](void *vec, int const idx, const char** out_text) {
    auto *ppairs = reinterpret_cast<std::vector<pair_t>*>(vec);
    auto const index_size = static_cast<std::size_t>(idx);

    if (idx < 0 || index_size >= ppairs->size()) {
      return false;
    }
    *out_text = ppairs->at(idx).first.c_str();
    return true;
  };
  auto const find_entity = [](int const selected, auto const& pairs) {
    auto const cmp = [&selected](auto const& pair) {
      return pair.second == static_cast<std::uint32_t>(selected);
    };
    auto const find_it = std::find_if(pairs.cbegin(), pairs.cend(), cmp);
    assert(find_it != pairs.cend());
    return find_it->second;
  };
  std::vector<pair_t> pairs;
  for(auto const entity : registry.view<Transform>())
  {
    auto pair = std::make_pair(std::to_string(entity), entity);
    pairs.emplace_back(MOVE(pair));
  }

  if (ImGui::Begin("Entity Editor Window")) {
    auto &selected_entity = state.engine_state.ui_state.selected_entity;
    int selected = static_cast<int>(selected_entity);

    void *pdata = reinterpret_cast<void*>(&pairs);
    if (ImGui::Combo("Entity", &selected, combo_callback, pdata, pairs.size())) {
      selected_entity = selected;

      state.engine_state.camera.set_target(selected_entity);
      state.engine_state.player.set_eid(selected_entity);
    }
    auto const entity = find_entity(selected, pairs);
    auto &transform = registry.get<Transform>(entity);
    ImGui::InputFloat3("pos:", glm::value_ptr(transform.translation));
    {
      auto buffer = glm::degrees(glm::eulerAngles(transform.rotation));
      if (ImGui::InputFloat3("rot:", glm::value_ptr(buffer))) {
        transform.rotation = glm::quat(buffer * (3.14159f / 180.f));
      }
    }
    ImGui::InputFloat3("scale:", glm::value_ptr(transform.scale));
    ImGui::End();
  }
}

void
draw_camera_info(GameState &state)
{
  auto &camera = state.engine_state.camera;
  auto &player = state.engine_state.player;

  if (ImGui::Begin("CAMERA INFO WINDOW")) {
    {
      auto const coords = camera.spherical_coordinates();
      auto const r = coords.radius_display_string();
      auto const t = coords.theta_display_string();
      auto const p = coords.phi_display_string();
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
}

void
draw_player_info(GameState &state)
{
  auto &player = state.engine_state.player;

  ImGui::Begin("PLAYER INFO WINDOW");
  {
    auto const display = player.display();
    ImGui::Text("%s", display.c_str());


    glm::quat const quat = glm::angleAxis(glm::radians(0.0f), Y_UNIT_VECTOR);
    float const dot = glm::dot(player.orientation(), quat);
    std::string const dots = std::to_string(dot);
    ImGui::Text("dot product: '%s'", dots.c_str());
  }

  ImGui::End();
}

void
show_directionallight_window(GameState &state)
{
  auto &zone_state = state.zone_state;
  auto &directional = zone_state.global_light.directional;
  auto &ui_state = state.engine_state.ui_state;

  if (ImGui::Begin("Directional Light Editor")) {
    ImGui::Text("Directional Light");
    ImGui::InputFloat3("direction:", glm::value_ptr(directional.direction));

    auto &directional_light = directional.light;
    ImGui::ColorEdit3("Diffuse:", directional_light.diffuse.data());
    ImGui::ColorEdit3("Specular:", directional_light.specular.data());

    ImGui::Text("Attenuation");
    auto &attenuation = directional_light.attenuation;
    ImGui::InputFloat("constant:", &attenuation.constant);
    ImGui::InputFloat("linear:", &attenuation.linear);
    ImGui::InputFloat("quadratic:", &attenuation.quadratic, 0.0f, 1.0f);

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui_state.show_directionallight_window = false;
    }
    ImGui::End();
  }
}

void
show_ambientlight_window(GameState &state)
{
  auto &ui_state = state.engine_state.ui_state;
  if (ImGui::Begin("Global Light Editor")) {
    ImGui::Text("Global Light");
    auto &zone_state = state.zone_state;
    auto &global_light = zone_state.global_light;
    ImGui::ColorEdit3("Ambient Light Color:", global_light.ambient.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui_state.show_ambientlight_window = false;
    }
    ImGui::End();
  }
}

void
show_entitymaterials_window(GameState &state, entt::DefaultRegistry &registry)
{
  auto &ui_state = state.engine_state.ui_state;
  auto &selected_material = ui_state.selected_material;

  auto const combo_callback = [](void *vec, int const idx, const char** out_text) {
    auto *pvector = reinterpret_cast<std::vector<std::string>*>(vec);
    auto const index_size = static_cast<std::size_t>(idx);

    if (idx < 0 || index_size >= pvector->size()) {
      return false;
    }
    *out_text = pvector->at(idx).c_str();
    return true;
  };

  if (ImGui::Begin("Entity Materials Editor")) {
    auto const entities_with_materials = find_materials(registry);
    {
      std::vector<std::string> text;
      assert(!entities_with_materials.empty());

      for(std::uint32_t const e : entities_with_materials) {
        text.emplace_back(std::to_string(e));
      }

      void *pdata = reinterpret_cast<void*>(&text);
      auto const num_data = text.size();
      int selected = static_cast<int>(selected_material);
      if (ImGui::Combo("Entity", &selected, combo_callback, pdata, num_data)) {
        selected_material = selected;
      }
    }

    auto const& selected_entity = entities_with_materials[selected_material];
    auto const entity_o = find_entity_with_component<Material>(selected_entity, registry);
    assert(boost::none != entity_o);
    auto &material = registry.get<Material>(*entity_o);

    ImGui::Separator();
    ImGui::Text("Entity Material:");
    ImGui::ColorEdit3("ambient:", glm::value_ptr(material.ambient));
    ImGui::ColorEdit3("diffuse:", glm::value_ptr(material.diffuse));
    ImGui::ColorEdit3("specular:", glm::value_ptr(material.specular));
    ImGui::SliderFloat("shininess:", &material.shininess, 0.0f, 1.0f);

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui_state.show_entitymaterial_window = false;
    }
    ImGui::End();
  }
}

void
show_pointlight_window(GameState &state, entt::DefaultRegistry &registry)
{
  auto const display_pointlight = [&registry](auto const index, auto const& pointlights) {
    auto const& entity = pointlights[index];
    auto &transform = registry.get<Transform>(entity);
    auto &pointlight = registry.get<PointLight>(entity);
    std::string const text = "PointLight #" + std::to_string(index);
    ImGui::Text("PointLight #%s", std::to_string(index).c_str());

    auto &light = pointlight.light;
    ImGui::InputFloat3("position:", glm::value_ptr(transform.translation));
    ImGui::ColorEdit3("diffuse:", light.diffuse.data());
    ImGui::ColorEdit3("specular:", light.specular.data());
    ImGui::Separator();

    ImGui::Text("Attenuation");
    auto &attenuation = pointlight.light.attenuation;
    ImGui::SliderFloat("constant:", &attenuation.constant, 0.0f, 1.0f);
    ImGui::SliderFloat("linear:", &attenuation.linear, 0.0f, 1.0f);
    ImGui::SliderFloat("quadratic:", &attenuation.quadratic, 0.0f, 1.0f);
  };

  auto &ui_state = state.engine_state.ui_state;
  if (ImGui::Begin("Pointlight Editor")) {
    ImGui::InputInt("PointLight #:", &ui_state.selected_pointlight);
    auto const pointlights = find_pointlights(registry);

    display_pointlight(ui_state.selected_pointlight, pointlights);
    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui_state.show_pointlight_window = false;
    }
    ImGui::End();
  }
}

void
show_background_window(GameState &state)
{
  auto &engine_state = state.engine_state;
  auto &zone_state = state.zone_state;
  auto &ui_state = engine_state.ui_state;

  if (ImGui::Begin("Background Color")) {
    ImGui::ColorEdit3("Background Color:", zone_state.background.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui_state.show_background_window = false;
    }
    ImGui::End();
  }
}

void
world_menu(GameState &state)
{
  auto &engine_state = state.engine_state;
  auto &ui_state = engine_state.ui_state;

  if (ImGui::BeginMenu("World")) {
    ImGui::MenuItem("Background Color", nullptr, &ui_state.show_background_window);
    ImGui::MenuItem("Local Axis", nullptr, &engine_state.show_local_axis);
    ImGui::MenuItem("Global Axis", nullptr, &engine_state.show_global_axis);
    ImGui::MenuItem("Target Forward/Right/Up Vectors", nullptr, &engine_state.show_target_vectors);

    auto &tilemap_state = engine_state.tilemap_state;
    if (ImGui::MenuItem("Reveal Tilemap", nullptr, &tilemap_state.reveal)) {
      tilemap_state.redraw = true;
    }

    if (ImGui::BeginMenu("TileMap GridLines (Debug)")) {
      ImGui::MenuItem("Show (x, z)-axis lines", nullptr, &tilemap_state.show_grid_lines);
      if (ImGui::MenuItem("Show y-axis Lines ", nullptr, &tilemap_state.show_yaxis_lines)) {
        tilemap_state.redraw = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }

  if (ui_state.show_background_window) {
    show_background_window(state);
  }
}

void
lighting_menu(GameState &state, entt::DefaultRegistry &registry)
{
  auto &ui_state = state.engine_state.ui_state;
  bool &edit_pointlights = ui_state.show_pointlight_window;
  bool &edit_ambientlight = ui_state.show_ambientlight_window;
  bool &edit_directionallights = ui_state.show_directionallight_window;
  bool &edit_entitymaterials = ui_state.show_entitymaterial_window;

  if (ImGui::BeginMenu("Lighting")) {
    ImGui::MenuItem("Point-Lights", nullptr, &edit_pointlights);
    ImGui::MenuItem("Ambient Lighting", nullptr, &edit_ambientlight);
    ImGui::MenuItem("Directional Lighting", nullptr, &edit_directionallights);
    ImGui::MenuItem("Entity Materials", nullptr, &edit_entitymaterials);
    ImGui::EndMenu();
  }
  if (edit_pointlights) {
    show_pointlight_window(state, registry);
  }
  if (edit_ambientlight) {
    show_ambientlight_window(state);
  }
  if (edit_directionallights) {
    show_directionallight_window(state);
  }
  if (edit_entitymaterials) {
    show_entitymaterials_window(state, registry);
  }
}

} // ns anonymous

namespace boomhs
{

void
draw_ui(GameState &state, window::SDLWindow &window, entt::DefaultRegistry &registry)
{
  auto &engine_state = state.engine_state;
  auto &ui_state = engine_state.ui_state;
  auto &window_state = engine_state.window_state;

  if (ui_state.show_entitywindow) {
    draw_entity_editor(state, registry);
  }
  if (ui_state.show_camerawindow) {
    draw_camera_info(state);
  }
  if (ui_state.show_playerwindow) {
    draw_player_info(state);
  }
  if (ui_state.show_debugwindow) {
    ImGui::Checkbox("Draw Skybox", &engine_state.draw_skybox);
    ImGui::Checkbox("Enter Pressed", &ui_state.enter_pressed);
    ImGui::Checkbox("Mouse Rotation Lock", &ui_state.rotate_lock);
    ImGui::Checkbox("Draw Entities", &engine_state.draw_entities);
    ImGui::Checkbox("Draw Tilemap", &engine_state.draw_tilemap);
    ImGui::Checkbox("Draw Normals", &engine_state.draw_normals);
    ImGui::Checkbox("Player Collisions Enabled", &engine_state.player_collision);
  }

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      ImGui::MenuItem("Debug Menu", nullptr, &ui_state.show_debugwindow);
      ImGui::MenuItem("Entity Menu", nullptr, &ui_state.show_entitywindow);
      ImGui::MenuItem("Camera Menu", nullptr, &ui_state.show_camerawindow);
      ImGui::MenuItem("Player Menu", nullptr, &ui_state.show_playerwindow);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Settings")) {
      auto const draw_row = [&](char const* text, auto const fullscreen) {
        if (ImGui::MenuItem(text, nullptr, nullptr, window_state.fullscreen != fullscreen)) {
          window.set_fullscreen(fullscreen);
          window_state.fullscreen = fullscreen;
        }
      };
      draw_row("NOT Fullscreen", window::FullscreenFlags::NOT_FULLSCREEN);
      draw_row("Fullscreen", window::FullscreenFlags::FULLSCREEN);
      draw_row("Fullscreen DESKTOP", window::FullscreenFlags::FULLSCREEN_DESKTOP);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Camera")) {
      ImGui::MenuItem("Flip Y Sensitivity", nullptr, &ui_state.flip_y);
      ImGui::EndMenu();
    }
    world_menu(state);
    lighting_menu(state, registry);

    auto const framerate = engine_state.imgui.Framerate;
    auto const ms_frame = 1000.0f / framerate;
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.76f);
    ImGui::Text("FPS(avg): %.1f ms/frame: %.3f", framerate, ms_frame);
    ImGui::EndMainMenuBar();
  }
}

} // ns anonymous
