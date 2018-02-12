#include <boomhs/ui.hpp>
#include <boomhs/state.hpp>
#include <boomhs/zone.hpp>

#include <window/sdl_window.hpp>

#include <stlw/math.hpp>
#include <stlw/format.hpp>

#include <imgui/imgui.hpp>
#include <algorithm>

namespace
{
using namespace boomhs;
using namespace opengl;

using pair_t = std::pair<std::string, std::uint32_t>;

template<typename ...T>
auto
collect_all(entt::DefaultRegistry &registry)
{
  std::vector<pair_t> pairs;
  for(auto const entity : registry.view<T...>())
  {
    auto pair = std::make_pair(std::to_string(entity), entity);
    pairs.emplace_back(MOVE(pair));
  }
  std::reverse(pairs.begin(), pairs.end());
  return pairs;
}

auto
callback_from_pairs(void *const pvec, int const idx, const char** out_text)
{
  std::vector<pair_t> const& vec = *reinterpret_cast<std::vector<pair_t>*>(pvec);

  auto const index_size = static_cast<std::size_t>(idx);
  if (idx < 0 || index_size >= vec.size()) {
    return false;
  }
  auto const& pair = vec[idx];
  auto const& name = pair.first;
  *out_text = name.c_str();
  return true;
};

auto
callback_from_strings(void *const pvec, int const idx, const char** out_text)
{
  auto const& vec = *reinterpret_cast<std::vector<std::string>*>(pvec);

  auto const index_size = static_cast<std::size_t>(idx);
  if (idx < 0 || index_size >= vec.size()) {
    return false;
  }
  *out_text = vec[idx].c_str();
  return true;
};

template<typename T>
bool
display_combo_for_entities(char const* text, int *selected, entt::DefaultRegistry &registry,
    std::vector<T> &pairs)
{
  void *pdata = reinterpret_cast<void *>(&pairs);
  return ImGui::Combo(text, selected, callback_from_pairs, pdata, pairs.size());
}

std::uint32_t
comboselected_to_eid(int const selected_index, std::vector<pair_t> const& pairs)
{
  auto const selected_string = std::to_string(selected_index);
  auto const cmp = [&selected_string](auto const& pair) { return pair.first == selected_string; };
  auto const it = std::find_if(pairs.cbegin(), pairs.cend(), cmp);
  assert(it != pairs.cend());

  return it->second;
}

void
draw_entity_editor(UiState &uistate, ZoneState &zone_state, entt::DefaultRegistry &registry)
{
  auto &selected = uistate.selected_entity;
  if (ImGui::Begin("Entity Editor Window")) {
    auto pairs = collect_all<Transform>(registry);
    if (display_combo_for_entities("Entity", &selected, registry, pairs)) {
      auto const eid = comboselected_to_eid(selected, pairs);

      auto &player = zone_state.player;
      auto &camera = zone_state.camera;

      camera.set_target(eid);
      player.set_eid(eid);
    }
    auto const eid = comboselected_to_eid(selected, pairs);
    auto &transform = registry.get<Transform>(eid);
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
draw_tiledata_editor(TiledataState &tds, ZoneManager &zm)
{
  if (ImGui::Begin("Tilemap Editor Window")) {
    ImGui::InputFloat3("Floor Offset:", glm::value_ptr(tds.floor_offset));
    ImGui::InputFloat3("Tile Scaling:", glm::value_ptr(tds.tile_scaling));

    std::vector<std::string> levels;
    FORI(i, zm.num_zones()) {
      levels.emplace_back(std::to_string(i));
    }
    void *pdata = reinterpret_cast<void *>(&levels);
    int selected = zm.active_zone();

    if (ImGui::Combo("Current Level:", &selected, callback_from_strings, pdata, zm.num_zones())) {
      zm.make_zone_active(selected, tds);
    }
    bool recompute = false;
    recompute |= ImGui::Checkbox("Draw Tilemap", &tds.draw_tiledata);
    recompute |= ImGui::Checkbox("Reveal Tilemap Hidden", &tds.reveal);
    recompute |= ImGui::Checkbox("Show (x, z)-axis lines", &tds.show_grid_lines);
    recompute |= ImGui::Checkbox("Show y-axis Lines ", &tds.show_yaxis_lines);
    ImGui::Checkbox("Draw Neighbor Arrows", &tds.show_neighbortile_arrows);

    if (recompute) {
      tds.recompute = true;
    }
    ImGui::End();
  }
}

void
draw_camera_window(ZoneState &zone_state)
{
  auto &player = zone_state.player;
  auto &camera = zone_state.camera;

  auto const draw_perspective_controls = [&]()
  {
    ImGui::Text("Perspective Projection");
    ImGui::Separator();
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
    ImGui::Text("camera xyz: '%s', camera phi: '%s', camera theta: '%s'\n",
        glm::to_string(camera.world_position()).c_str(),
        std::to_string(camera.spherical_coordinates().phi).c_str(),
        std::to_string(camera.spherical_coordinates().theta).c_str()
        );
    ImGui::Separator();
    ImGui::Separator();
    auto &perspective = camera.perspective_ref();
    ImGui::InputFloat("Field of View", &perspective.field_of_view);
    ImGui::InputFloat("Near Plane", &perspective.near_plane);
    ImGui::InputFloat("Far Plane", &perspective.far_plane);
  };
  auto const draw_ortho_controls = [&]() {
    ImGui::Text("Orthographic Projection");
    ImGui::Separator();
    auto &ortho = camera.ortho_ref();
    ImGui::InputFloat("Left:", &ortho.left);
    ImGui::InputFloat("Right:", &ortho.right);
    ImGui::InputFloat("Bottom:", &ortho.bottom);
    ImGui::InputFloat("Top:", &ortho.top);
    ImGui::InputFloat("Far:", &ortho.far);
    ImGui::InputFloat("Near:", &ortho.near);
  };
  if (ImGui::Begin("CAMERA INFO WINDOW")) {
    ImGui::Checkbox("Flip Y Sensitivity", &camera.flip_y);
    ImGui::Checkbox("Mouse Rotation Lock", &camera.rotate_lock);
    ImGui::InputFloat("Mouse Rotation Speed", &camera.rotation_speed);
    std::vector<std::string> mode_strings;
    for(auto const& it : CAMERA_MODES) {
      mode_strings.emplace_back(it.second);
    }
    int selected = static_cast<int>(camera.mode());;
    void *pdata = reinterpret_cast<void *>(&mode_strings);
    if (ImGui::Combo("Mode:", &selected, callback_from_strings, pdata, mode_strings.size())) {
      auto const mode = static_cast<CameraMode>(selected);
      camera.set_mode(mode);
    }
    switch(camera.mode()) {
      case Perspective: {
        draw_perspective_controls();
        break;
      }
      case Ortho: {
        draw_ortho_controls();
        break;
      }
      default: {
        break;
      }
    }
    ImGui::End();
  }
}

void
draw_mouse_window(MouseState &mstate)
{
  if (ImGui::Begin("MOUSE INFO WINDOW")) {
    ImGui::InputFloat("X sensitivity:", &mstate.sensitivity.x, 0.0f, 1.0f);
    ImGui::InputFloat("Y sensitivity:", &mstate.sensitivity.y, 0.0f, 1.0f);
    ImGui::End();
  }
}

void
draw_player_window(EngineState &es, ZoneState &zone_state)
{
  auto &player = zone_state.player;

  if (ImGui::Begin("PLAYER INFO WINDOW")) {
    auto const display = player.display();
    ImGui::Text("%s", display.c_str());

    ImGui::Checkbox("Player Collisions Enabled", &es.player_collision);
    ImGui::Checkbox("Localspace Vectors Vectors", &es.show_player_localspace_vectors);
    ImGui::Checkbox("Worldspace Vectors Vectors", &es.show_player_worldspace_vectors);
    float speed = player.speed();
    if (ImGui::InputFloat("Player Speed", &speed)) {
      player.set_speed(speed);
    }

    glm::quat const quat = glm::angleAxis(glm::radians(0.0f), Y_UNIT_VECTOR);
    float const dot = glm::dot(player.orientation(), quat);
    std::string const dots = std::to_string(dot);
    ImGui::Text("dot product: '%s'", dots.c_str());
    ImGui::End();
  }
}

void
show_directionallight_window(UiState &ui, ZoneState &zone_state)
{
  auto &directional = zone_state.global_light.directional;

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
      ui.show_directionallight_window = false;
    }
    ImGui::End();
  }
}

void
show_ambientlight_window(UiState &ui, ZoneState &zone_state)
{
  if (ImGui::Begin("Global Light Editor")) {
    ImGui::Text("Global Light");

    auto &global_light = zone_state.global_light;
    ImGui::ColorEdit3("Ambient Light Color:", global_light.ambient.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui.show_ambientlight_window = false;
    }
    ImGui::End();
  }
}

void
show_material_editor(char const* text, Material &material)
{
  ImGui::Text("%s", text);
  ImGui::ColorEdit3("ambient:", glm::value_ptr(material.ambient));
  ImGui::ColorEdit3("diffuse:", glm::value_ptr(material.diffuse));
  ImGui::ColorEdit3("specular:", glm::value_ptr(material.specular));
  ImGui::SliderFloat("shininess:", &material.shininess, 0.0f, 1.0f);
}

void
show_entitymaterials_window(UiState &ui, entt::DefaultRegistry &registry)
{
  auto &selected_material = ui.selected_entity_material;

  if (ImGui::Begin("Entity Materials Editor")) {
    auto pairs = collect_all<Material, Transform>(registry);
    display_combo_for_entities<>("Entity", &selected_material, registry, pairs);

    auto const entities_with_materials = find_materials(registry);
    auto const& selected_entity = entities_with_materials[selected_material];

    auto &material = registry.get<Material>(selected_entity);
    ImGui::Separator();
    show_material_editor("Entity Material:", material);

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui.show_entitymaterial_window = false;
    }
    ImGui::End();
  }
}

void
show_tiledata_materials_window(UiState &ui, TileData &tdata, entt::DefaultRegistry &registry)
{
  if (ImGui::Begin("Entity Materials Editor")) {

    // 1. Collect Tile names
    std::vector<std::string> tile_names;
    FOR(i, static_cast<size_t>(TileType::MAX)) {
      auto const type = static_cast<TileType>(i);
      tile_names.emplace_back(to_string(type));
    }

    // 2. Collect entities
    /*
    std::vector<Material*> walls;
    for(auto &it : tdata) {
      if (it.type == TileType::WALL) {
        auto const eid = it.eid;
        assert(registry.has<Material>(eid));
        Material &material = registry.get<Material>(eid);
        walls.emplace_back(&material);
      }
    }
    */

    auto *selected_tile = ui.selected_tile.data();
    ImGui::InputInt2("TilePosition:", selected_tile);
    assert(selected_tile);
    auto const st = static_cast<uint64_t>(*selected_tile);

    auto &tile = tdata.data(TilePosition{st});
    assert(registry.has<Material>(tile.eid));
    auto &selected_material = registry.get<Material>(tile.eid);
    show_material_editor("Tile Material:", selected_material);
    int &selected = ui.selected_tiledata;

    {
      void *pdata = reinterpret_cast<void *>(&tile_names);
      ImGui::Combo("Tile Type:", &selected, callback_from_strings, pdata, tile_names.size());
      ImGui::Separator();
    }
    if (ImGui::Button("Apply all")) {
      for(auto const& tile: tdata) {
        assert(registry.has<Material>(tile.eid));
        auto const selected_type = tiletype_from_string(tile_names[selected]);
        if (tile.type == selected_type) {
          registry.get<Material>(tile.eid) = selected_material;//.ambient = glm::vec3{0.0, 0.0, 0.0};// = selected_material;;
        }
        //registry.get<Material>(tile.eid).diffuse = glm::vec3{0.0, 0.0, 0.0};// = selected_material;;
        //registry.get<Material>(tile.eid).specular = glm::vec3{0.0, 0.0, 0.0};// = selected_material;;
        std::cerr << "applying ...\n";
      }
    }
    ImGui::End();
  }
}

void
show_pointlight_window(UiState &ui, entt::DefaultRegistry &registry)
{
  auto const display_pointlight = [&registry](std::uint32_t const entity) {
    auto &transform = registry.get<Transform>(entity);
    auto &pointlight = registry.get<PointLight>(entity);
    auto &light = pointlight.light;
    ImGui::InputFloat3("position:", glm::value_ptr(transform.translation));
    ImGui::ColorEdit3("diffuse:", light.diffuse.data());
    ImGui::ColorEdit3("specular:", light.specular.data());
    ImGui::Separator();

    ImGui::Text("Attenuation");
    auto &attenuation = pointlight.light.attenuation;
    ImGui::InputFloat("constant:", &attenuation.constant);
    ImGui::InputFloat("linear:", &attenuation.linear);
    ImGui::InputFloat("quadratic:", &attenuation.quadratic);
  };
  if (ImGui::Begin("Pointlight Editor")) {
    auto &selected_pointlight = ui.selected_pointlight;
    auto pairs = collect_all<PointLight, Transform>(registry);
    display_combo_for_entities<>("PointLight:", &selected_pointlight, registry, pairs);

    auto const pointlights = find_pointlights(registry);
    display_pointlight(pointlights[selected_pointlight]);

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui.show_pointlight_window = false;
    }
    ImGui::End();
  }
}

void
show_background_window(UiState &ui_state, ZoneState &zone_state)
{
  if (ImGui::Begin("Background Color")) {
    ImGui::ColorEdit3("Background Color:", zone_state.background.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui_state.show_background_window = false;
    }
    ImGui::End();
  }
}

void
world_menu(EngineState &es, ZoneState &zone_state)
{
  auto &ui = es.ui_state;
  if (ImGui::BeginMenu("World")) {
    ImGui::MenuItem("Background Color", nullptr, &ui.show_background_window);
    ImGui::MenuItem("Local Axis", nullptr, &es.show_local_axis);
    ImGui::MenuItem("Global Axis", nullptr, &es.show_global_axis);
    ImGui::EndMenu();
  }

  if (ui.show_background_window) {
    show_background_window(ui, zone_state);
  }
}

void
lighting_menu(EngineState &es, ZoneState &zone_state, entt::DefaultRegistry &registry)
{
  auto &ui = es.ui_state;
  bool &edit_pointlights = ui.show_pointlight_window;
  bool &edit_ambientlight = ui.show_ambientlight_window;
  bool &edit_directionallights = ui.show_directionallight_window;
  bool &edit_entitymaterials = ui.show_entitymaterial_window;
  bool &edit_tiledatamaterials = ui.show_tiledatamaterial_window;

  if (ImGui::BeginMenu("Lighting")) {
    ImGui::MenuItem("Point-Lights", nullptr, &edit_pointlights);
    ImGui::MenuItem("Ambient Lighting", nullptr, &edit_ambientlight);
    ImGui::MenuItem("Directional Lighting", nullptr, &edit_directionallights);
    ImGui::MenuItem("Entity Materials", nullptr, &edit_entitymaterials);
    ImGui::MenuItem("TileData Materials", nullptr, &edit_tiledatamaterials);
    ImGui::EndMenu();
  }
  if (edit_pointlights) {
    show_pointlight_window(ui, registry);
  }
  if (edit_ambientlight) {
    show_ambientlight_window(ui, zone_state);
  }
  if (edit_directionallights) {
    show_directionallight_window(ui, zone_state);
  }
  if (edit_entitymaterials) {
    show_entitymaterials_window(ui, registry);
  }
  if (edit_tiledatamaterials) {
    auto &tiledata = zone_state.level_data.tiledata_mutref();
    show_tiledata_materials_window(ui, tiledata, registry);
  }
}

} // ns anonymous

namespace boomhs
{

void
draw_ui(EngineState &es, ZoneManager &zm, window::SDLWindow &window, entt::DefaultRegistry &registry)
{
  auto &ui_state = es.ui_state;
  auto &tiledata_state = es.tiledata_state;
  auto &window_state = es.window_state;
  auto &zone_state = zm.active();

  if (ui_state.show_entitywindow) {
    draw_entity_editor(ui_state, zone_state, registry);
  }
  if (ui_state.show_camerawindow) {
    draw_camera_window(zone_state);
  }
  if (ui_state.show_mousewindow) {
    draw_mouse_window(es.mouse_state);
  }
  if (ui_state.show_playerwindow) {
    draw_player_window(es, zone_state);
  }
  if (ui_state.show_tiledata_editor_window) {
    draw_tiledata_editor(tiledata_state, zm);
  }
  if (ui_state.show_debugwindow) {
    ImGui::Checkbox("Draw Skybox", &es.draw_skybox);
    ImGui::Checkbox("Draw Terrain", &es.draw_terrain);
    ImGui::Checkbox("Enter Pressed", &ui_state.enter_pressed);
    ImGui::Checkbox("Draw Entities", &es.draw_entities);
    ImGui::Checkbox("Draw Normals", &es.draw_normals);
    ImGui::Checkbox("Mariolike Edges", &es.mariolike_edges);
  }

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Windows")) {
      ImGui::MenuItem("Debug Menu", nullptr, &ui_state.show_debugwindow);
      ImGui::MenuItem("Entity Menu", nullptr, &ui_state.show_entitywindow);
      ImGui::MenuItem("Camera Menu", nullptr, &ui_state.show_camerawindow);
      ImGui::MenuItem("Mouse Menu", nullptr, &ui_state.show_mousewindow);
      ImGui::MenuItem("Player Menu", nullptr, &ui_state.show_playerwindow);
      ImGui::MenuItem("Tilemap Menu", nullptr, &ui_state.show_tiledata_editor_window);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Settings")) {
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
      ImGui::EndMenu();
    }
    world_menu(es, zone_state);
    lighting_menu(es, zone_state, registry);

    auto const framerate = es.imgui.Framerate;
    auto const ms_frame = 1000.0f / framerate;

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
    ImGui::Text("Player Position: %s", glm::to_string(zone_state.player.world_position()).c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.60f);
    ImGui::Text("Current Level: %i", zm.active_zone());

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.76f);
    ImGui::Text("FPS(avg): %.1f ms/frame: %.3f", framerate, ms_frame);
    ImGui::EndMainMenuBar();
  }
}

} // ns anonymous
