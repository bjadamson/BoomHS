#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/state.hpp>
#include <boomhs/level_manager.hpp>
#include <opengl/global.hpp>

#include <extlibs/sdl.hpp>

#include <stlw/math.hpp>
#include <extlibs/fmt.hpp>

#include <extlibs/imgui.hpp>
#include <algorithm>
#include <optional>

namespace
{
using namespace boomhs;
using namespace opengl;

using pair_t = std::pair<std::string, EntityID>;

template<typename ...T>
auto
collect_all(EntityRegistry &registry)
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

  auto const index_size = static_cast<size_t>(idx);
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

  auto const index_size = static_cast<size_t>(idx);
  if (idx < 0 || index_size >= vec.size()) {
    return false;
  }
  *out_text = vec[idx].c_str();
  return true;
};

template<typename T>
bool
display_combo_for_entities(char const* text, int *selected, EntityRegistry &registry,
    std::vector<T> &pairs)
{
  void *pdata = reinterpret_cast<void *>(&pairs);
  return ImGui::Combo(text, selected, callback_from_pairs, pdata, pairs.size());
}

EntityID
comboselected_to_entity(int const selected_index, std::vector<pair_t> const& pairs)
{
  auto const selected_string = std::to_string(selected_index);
  auto const cmp = [&selected_string](auto const& pair) { return pair.first == selected_string; };
  auto const it = std::find_if(pairs.cbegin(), pairs.cend(), cmp);
  assert(it != pairs.cend());

  return it->second;
}

void
draw_entity_editor(UiDebugState &uistate, LevelData &ldata, EntityRegistry &registry)
{
  auto &selected = uistate.selected_entity;
  auto const draw = [&]() {
    auto pairs = collect_all<Transform>(registry);
    if (display_combo_for_entities("Entity", &selected, registry, pairs)) {
      auto const eid = comboselected_to_entity(selected, pairs);

      auto &player = ldata.player;
      auto &camera = ldata.camera;

      camera.set_target(eid);
      player.set_eid(eid);
    }
    auto const eid = comboselected_to_entity(selected, pairs);
    auto &transform = registry.get<Transform>(eid);
    ImGui::InputFloat3("pos:", glm::value_ptr(transform.translation));
    {
      auto buffer = glm::degrees(glm::eulerAngles(transform.rotation));
      if (ImGui::InputFloat3("rot:", glm::value_ptr(buffer))) {
        transform.rotation = glm::quat(buffer * (3.14159f / 180.f));
      }
    }
    ImGui::InputFloat3("scale:", glm::value_ptr(transform.scale));
  };
  imgui_cxx::with_window(draw, "Entity Editor Window");
}

void
draw_tilegrid_editor(TiledataState &tds, LevelManager &lm)
{
  auto const draw = [&]() {
    ImGui::InputFloat3("Floor Offset:", glm::value_ptr(tds.floor_offset));
    ImGui::InputFloat3("Tile Scaling:", glm::value_ptr(tds.tile_scaling));

    std::vector<std::string> levels;
    FORI(i, lm.num_levels()) {
      levels.emplace_back(std::to_string(i));
    }
    void *pdata = reinterpret_cast<void *>(&levels);
    int selected = lm.active_zone();

    if (ImGui::Combo("Current Level:", &selected, callback_from_strings, pdata, lm.num_levels())) {
      lm.make_active(selected, tds);
    }
    bool recompute = false;
    recompute |= ImGui::Checkbox("Draw Tilemap", &tds.draw_tilegrid);
    recompute |= ImGui::Checkbox("Reveal Tilemap Hidden", &tds.reveal);
    recompute |= ImGui::Checkbox("Show (x, z)-axis lines", &tds.show_grid_lines);
    recompute |= ImGui::Checkbox("Show y-axis Lines ", &tds.show_yaxis_lines);
    ImGui::Checkbox("Draw Neighbor Arrows", &tds.show_neighbortile_arrows);

    if (recompute) {
      tds.recompute = true;
    }
  };
  imgui_cxx::with_window(draw, "Tilemap Editor Window");
}

void
draw_camera_window(LevelData &ldata)
{
  auto &player = ldata.player;
  auto &camera = ldata.camera;

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
  auto const draw_camera_window = [&]() {
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
      case Perspective:
        draw_perspective_controls();
        break;
      case Ortho:
        draw_ortho_controls();
        break;
      case FPS:
        // TODO: implement this
        std::abort();
        break;
      default: {
        break;
      }
    }
  };
  imgui_cxx::with_window(draw_camera_window, "CAMERA INFO WINDOW");
}

void
draw_mouse_window(MouseState &mstate)
{
  auto const draw = [&]() {
    ImGui::InputFloat("X sensitivity:", &mstate.sensitivity.x, 0.0f, 1.0f);
    ImGui::InputFloat("Y sensitivity:", &mstate.sensitivity.y, 0.0f, 1.0f);
  };
  imgui_cxx::with_window(draw, "MOUSE INFO WINDOW");
}

void
draw_player_window(EngineState &es, LevelData &ldata)
{
  auto &player = ldata.player;

  auto const draw = [&]() {
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
  };
  imgui_cxx::with_window(draw, "PLAYER INFO WINDOW");
}

void
show_directionallight_window(UiDebugState &ui, LevelData &ldata)
{
  auto &directional = ldata.global_light.directional;

  auto const draw = [&]() {
    ImGui::Text("Directional Light");
    ImGui::InputFloat3("direction:", glm::value_ptr(directional.direction));

    auto &directional_light = directional.light;
    ImGui::ColorEdit3("Diffuse:", directional_light.diffuse.data());
    ImGui::ColorEdit3("Specular:", directional_light.specular.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui.show_directionallight_window = false;
    }
  };
  imgui_cxx::with_window(draw, "Directional Light Editor");
}

void
show_ambientlight_window(UiDebugState &ui, LevelData &ldata)
{
  auto const draw = [&]() {
    ImGui::Text("Global Light");

    auto &global_light = ldata.global_light;
    ImGui::ColorEdit3("Ambient Light Color:", global_light.ambient.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui.show_ambientlight_window = false;
    }
  };
  imgui_cxx::with_window(draw, "Global Light Editor");
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
show_entitymaterials_window(UiDebugState &ui, EntityRegistry &registry)
{
  auto &selected_material = ui.selected_entity_material;

  auto const draw = [&]() {
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
  };
  imgui_cxx::with_window(draw, "Entity Materials Editor");
}

void
show_tilegrid_materials_window(UiDebugState &ui, LevelData &level_data)
{
  auto const draw = [&]() {
    // 1. Collect Tile names
    std::vector<std::string> tile_names;
    FOR(i, static_cast<size_t>(TileType::UNDEFINED)) {
      auto const type = static_cast<TileType>(i);
      tile_names.emplace_back(to_string(type));
    }

    int &selected = ui.selected_tilegrid;
    assert(selected < static_cast<int>(tile_names.size()));

    TileInfo &tileinfo = level_data.tiletable()[static_cast<TileType>(selected)];
    auto &material = tileinfo.material;
    show_material_editor("Tile Material:", material);
    {
      void *pdata = reinterpret_cast<void *>(&tile_names);
      ImGui::Combo("Tile Type:", &selected, callback_from_strings, pdata, tile_names.size());
      ImGui::Separator();
    }
  };
  imgui_cxx::with_window(draw, "Tilegrid Materials Editor");
}

void
show_pointlight_window(UiDebugState &ui, EntityRegistry &registry)
{
  auto const display_pointlight = [&registry](EntityID const entity) {
    auto &transform = registry.get<Transform>(entity);
    auto &pointlight = registry.get<PointLight>(entity);
    auto &light = pointlight.light;
    ImGui::InputFloat3("position:", glm::value_ptr(transform.translation));
    ImGui::ColorEdit3("diffuse:", light.diffuse.data());
    ImGui::ColorEdit3("specular:", light.specular.data());
    ImGui::Separator();

    ImGui::Text("Attenuation");
    auto &attenuation = pointlight.attenuation;
    ImGui::InputFloat("constant:", &attenuation.constant);
    ImGui::InputFloat("linear:", &attenuation.linear);
    ImGui::InputFloat("quadratic:", &attenuation.quadratic);

    if (registry.has<LightFlicker>(entity)) {
      ImGui::Separator();
      ImGui::Text("Light Flicker");

      auto &flicker = registry.get<LightFlicker>(entity);
      ImGui::InputFloat("speed:", &flicker.current_speed);

      FOR(i, flicker.colors.size()) {
        auto &light = flicker.colors[i];
        ImGui::ColorEdit4("color:", light.data(), ImGuiColorEditFlags_Float);
        ImGui::Separator();
      }
    }
  };
  auto const draw_pointlight_editor = [&]() {
    auto &selected_pointlight = ui.selected_pointlight;
    auto pairs = collect_all<PointLight, Transform>(registry);
    display_combo_for_entities<>("PointLight:", &selected_pointlight, registry, pairs);
    ImGui::Separator();

    auto const pointlights = find_pointlights(registry);
    display_pointlight(pointlights[selected_pointlight]);

    if (ImGui::Button("Close", ImVec2(120,0))) {
      ui.show_pointlight_window = false;
    }
  };
  imgui_cxx::with_window(draw_pointlight_editor, "Pointlight Editor");
}

void
show_background_window(UiDebugState &state, LevelData &ldata)
{
  auto const draw = [&]() {
    ImGui::ColorEdit3("Background Color:", ldata.background.data());

    if (ImGui::Button("Close", ImVec2(120,0))) {
      state.show_background_window = false;
    }
  };
  imgui_cxx::with_window(draw, "Background Color");
}

void
world_menu(EngineState &es, LevelData &ldata)
{
  auto &ui = es.ui_state.debug;
  auto const draw = [&]() {
    ImGui::MenuItem("Background Color", nullptr, &ui.show_background_window);
    ImGui::MenuItem("Local Axis", nullptr, &es.show_local_axis);
    ImGui::MenuItem("Global Axis", nullptr, &es.show_global_axis);

    if (ui.show_background_window) {
      show_background_window(ui, ldata);
    }
  };
  imgui_cxx::with_menu(draw, "World");
}

void
lighting_menu(EngineState &es, LevelData &ldata, EntityRegistry &registry)
{
  auto &ui = es.ui_state.debug;
  bool &edit_pointlights = ui.show_pointlight_window;
  bool &edit_ambientlight = ui.show_ambientlight_window;
  bool &edit_directionallights = ui.show_directionallight_window;
  bool &edit_entitymaterials = ui.show_entitymaterial_window;
  bool &edit_tilegridmaterials = ui.show_tilegridmaterial_window;

  auto const draw = [&]() {
    ImGui::MenuItem("Point-Lights", nullptr, &edit_pointlights);
    ImGui::MenuItem("Ambient Lighting", nullptr, &edit_ambientlight);
    ImGui::MenuItem("Directional Lighting", nullptr, &edit_directionallights);
    ImGui::MenuItem("Entity Materials", nullptr, &edit_entitymaterials);
    ImGui::MenuItem("TileGrid Materials", nullptr, &edit_tilegridmaterials);
  };
  imgui_cxx::with_menu(draw, "Lightning");
  if (edit_pointlights) {
    show_pointlight_window(ui, registry);
  }
  if (edit_ambientlight) {
    show_ambientlight_window(ui, ldata);
  }
  if (edit_directionallights) {
    show_directionallight_window(ui, ldata);
  }
  if (edit_entitymaterials) {
    show_entitymaterials_window(ui, registry);
  }
  if (edit_tilegridmaterials) {
    show_tilegrid_materials_window(ui, ldata);
  }
}

} // ns anonymous

namespace boomhs::ui_debug
{

void
draw(EngineState &es, LevelManager &lm, window::SDLWindow &window)
{
  auto &state = es.ui_state.debug;
  auto &tilegrid_state = es.tilegrid_state;
  auto &window_state = es.window_state;
  auto &zs = lm.active();
  auto &registry = zs.registry;
  auto &ldata = zs.level_data;

  if (state.show_entitywindow) {
    draw_entity_editor(state, ldata, registry);
  }
  if (state.show_camerawindow) {
    draw_camera_window(ldata);
  }
  if (state.show_mousewindow) {
    draw_mouse_window(es.mouse_state);
  }
  if (state.show_playerwindow) {
    draw_player_window(es, ldata);
  }
  if (state.show_tilegrid_editor_window) {
    draw_tilegrid_editor(tilegrid_state, lm);
  }
  if (state.show_debugwindow) {
    ImGui::Checkbox("Draw Skybox", &es.draw_skybox);
    ImGui::Checkbox("Draw Terrain", &es.draw_terrain);
    ImGui::Checkbox("Enter Pressed", &state.enter_pressed);
    ImGui::Checkbox("Draw Entities", &es.draw_entities);
    ImGui::Checkbox("Draw Normals", &es.draw_normals);
    ImGui::Checkbox("Mariolike Edges", &es.mariolike_edges);

    ImGui::Checkbox("ImGui Metrics", &es.draw_imguimetrics);
    if (es.draw_imguimetrics) {
      ImGui::ShowMetricsWindow(&es.draw_imguimetrics);
    }
  }

  auto const windows_menu = [&]() {
    ImGui::MenuItem("Debug Menu", nullptr, &state.show_debugwindow);
    ImGui::MenuItem("Entity Menu", nullptr, &state.show_entitywindow);
    ImGui::MenuItem("Camera Menu", nullptr, &state.show_camerawindow);
    ImGui::MenuItem("Mouse Menu", nullptr, &state.show_mousewindow);
    ImGui::MenuItem("Player Menu", nullptr, &state.show_playerwindow);
    ImGui::MenuItem("Tilemap Menu", nullptr, &state.show_tilegrid_editor_window);
  };
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
  auto const draw_mainmenu = [&]() {
    imgui_cxx::with_menu(windows_menu, "Windows");
    imgui_cxx::with_menu(settings_menu, "Settings");
    world_menu(es, ldata);
    lighting_menu(es, ldata, registry);

    auto const framerate = es.imgui.Framerate;
    auto const ms_frame = 1000.0f / framerate;

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
    ImGui::Text("Player Position: %s", glm::to_string(ldata.player.world_position()).c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.60f);
    ImGui::Text("Current Level: %i", lm.active_zone());

    ImGui::SameLine(ImGui::GetWindowWidth() * 0.76f);
    ImGui::Text("FPS(avg): %.1f ms/frame: %.3f", framerate, ms_frame);
  };
  imgui_cxx::with_mainmenubar(draw_mainmenu);
}

} // ns boomhs::ui_debug
