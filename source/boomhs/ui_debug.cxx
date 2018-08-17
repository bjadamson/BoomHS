#include <boomhs/audio.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/player.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/time.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/view_frustum.hpp>
#include <boomhs/water.hpp>

#include <opengl/global.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox_renderer.hpp>

#include <boomhs/math.hpp>

#include <algorithm>
#include <optional>
#include <sstream>

#include <extlibs/fmt.hpp>
#include <extlibs/imgui.hpp>
#include <extlibs/sdl.hpp>

namespace
{
using namespace boomhs;
using namespace opengl;

using pair_t = std::pair<std::string, EntityID>;

template <typename... T>
auto
collect_name_eid_pairs(EntityRegistry& registry, bool const reverse = true)
{
  std::vector<pair_t> pairs;
  for (auto const eid : registry.view<T...>()) {
    std::string name = "NO NAME";
    if (registry.has<Name>(eid)) {
      name = registry.get<Name>(eid).value;
    }
    auto       pair = std::make_pair(name, eid);
    pairs.emplace_back(MOVE(pair));
  }
  if (reverse) {
    std::reverse(pairs.begin(), pairs.end());
  }
  return pairs;
}

auto
callback_from_strings(void* const pvec, int const idx, const char** out_text)
{
  auto const& vec = *reinterpret_cast<std::vector<std::string>*>(pvec);

  auto const index_size = static_cast<size_t>(idx);
  if (idx < 0 || index_size >= vec.size()) {
    return false;
  }
  *out_text = vec[idx].c_str();
  return true;
};


std::string
combine_names_into_one_string_forcombo(std::vector<pair_t> const& pairs)
{
  std::stringstream names;

  for (auto const name_eid : pairs) {
    auto const& name = name_eid.first;
    names << name << '\0';
  }
  names << '\0';
  return names.str();
}

bool
display_combo_for_pairs(char const* text, int* selected, std::vector<pair_t> const& pairs)
{
  auto const names = combine_names_into_one_string_forcombo(pairs);
  return ImGui::Combo(text, selected, names.c_str());
}

template <typename T, typename Table>
bool
table_combo(char const* text, T const& init, int* selected, std::string const& options,
            Table const& ttable)
{
  assert(selected);
  if (-1 == (*selected)) {
    auto lookup = ttable.index_of_nickname(init);
    *selected   = lookup.value_or(0);
  }
  return imgui_cxx::combo(text, selected, options);
}

} // namespace

namespace
{

void
draw_entity_editor(EngineState& es, LevelManager& lm, EntityRegistry& registry, Camera& camera,
                   glm::mat4 const& view_mat, glm::mat4 const& proj_mat)
{
  auto& logger    = es.logger;
  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& draw_handles = gfx_state.draw_handles;
  auto& sps       = gfx_state.sps;

  auto& uistate   = es.ui_state.debug;

  auto const draw = [&]() {
    std::optional<EntityID> selected;
    for (auto const eid : find_all_entities_with_component<Selectable>(registry)) {
      auto const& sel = registry.get<Selectable>(eid);
      if (sel.selected) {
        selected = eid;
        break;
      }
    }

    std::string const eid_str = selected ? std::to_string(*selected) : "none";
    ImGui::Text("eid: %s", eid_str.c_str());
    if (!selected) {
      return;
    }
    ImGui::Checkbox("Lock Selected", &uistate.lock_debugselected);
    auto const eid = *selected;

    {
      auto& isv = registry.get<IsVisible>(eid).value;
      ImGui::Checkbox("Visible", &isv);
    }

    if (registry.has<AABoundingBox>(eid) && registry.has<Transform>(eid)) {
      auto const& tr = registry.get<Transform>(eid);
      auto const& bbox = registry.get<AABoundingBox>(eid);

      // TODO: view/proj matrix
      bool const bbox_inside = ViewFrustum::bbox_inside(view_mat, proj_mat, tr, bbox);
      std::string const msg = fmt::sprintf("In ViewFrustum: %i", bbox_inside);
      ImGui::Text("%s", msg.c_str());
    }
    if (ImGui::Button("Inhabit Selected")) {
      auto& transform = registry.get<Transform>(eid);
      //camera.set_target(transform);
    }

    if (registry.has<Name>(eid)) {
      auto& name        = registry.get<Name>(eid).value;
      char  buffer[128] = {'0'};
      FOR(i, name.size()) { buffer[i] = name[i]; }
      ImGui::InputText(name.c_str(), buffer, IM_ARRAYSIZE(buffer));
    }
    if (registry.has<AABoundingBox>(eid)) {
      if (ImGui::CollapsingHeader("BoundingBox Editor")) {
        auto const& bbox = registry.get<AABoundingBox>(eid).cube;
        ImGui::Text("min: %s", glm::to_string(bbox.min).c_str());
        ImGui::Text("max: %s", glm::to_string(bbox.max).c_str());
      }
    }
    if (ImGui::CollapsingHeader("Transform Editor")) {
      auto& transform = registry.get<Transform>(eid);
      ImGui::InputFloat3("pos:", glm::value_ptr(transform.translation));
      {
        auto buffer = glm::degrees(glm::eulerAngles(transform.rotation));
        if (ImGui::InputFloat3("rot:", glm::value_ptr(buffer))) {
          transform.rotation = glm::quat(buffer * (3.14159f / 180.f));
        }
      }
      ImGui::InputFloat3("scale:", glm::value_ptr(transform.scale));
    }

    if (registry.has<TreeComponent>(eid) && ImGui::CollapsingHeader("Tree Editor")) {
      auto const make_str = [](char const* text, auto const num) {
        return text + std::to_string(num);
      };
      auto& tc = registry.get<TreeComponent>(eid);

      auto const edit_treecolor = [&](char const* name, auto const num_colors,
                                      auto const& get_color) {
        FOR(i, num_colors)
        {
          auto const text = make_str(name, i);
          ImGui::ColorEdit4(text.c_str(), get_color(i), ImGuiColorEditFlags_Float);
        }
      };

      edit_treecolor("Trunk", tc.num_trunks(),
                     [&tc](auto const i) { return tc.trunk_color(i).data(); });
      edit_treecolor("Stem", tc.num_stems(),
                     [&tc](auto const i) { return tc.stem_color(i).data(); });
      edit_treecolor("Leaves", tc.num_leaves(),
                     [&tc](auto const i) { return tc.leaf_color(i).data(); });

      auto& sn = registry.get<ShaderName>(eid);
      auto& va = sps.ref_sp(sn.value).va();

      auto& dinfo = draw_handles.lookup_entity(logger, eid);
      Tree::update_colors(logger, va, dinfo, tc);
    }

    if (registry.has<PointLight>(eid) && ImGui::CollapsingHeader("Pointlight")) {
      auto& transform  = registry.get<Transform>(eid);
      auto& pointlight = registry.get<PointLight>(eid);
      auto& light      = pointlight.light;
      ImGui::InputFloat3("position:", glm::value_ptr(transform.translation));
      ImGui::ColorEdit3("diffuse:", light.diffuse.data());
      ImGui::ColorEdit3("specular:", light.specular.data());
      ImGui::Separator();

      ImGui::Text("Attenuation");
      auto& attenuation = pointlight.attenuation;
      ImGui::InputFloat("constant:", &attenuation.constant);
      ImGui::InputFloat("linear:", &attenuation.linear);
      ImGui::InputFloat("quadratic:", &attenuation.quadratic);

      if (registry.has<LightFlicker>(eid)) {
        ImGui::Separator();
        ImGui::Text("Light Flicker");

        auto& flicker = registry.get<LightFlicker>(eid);
        ImGui::InputFloat("speed:", &flicker.current_speed);

        FOR(i, flicker.colors.size())
        {
          auto& light = flicker.colors[i];
          ImGui::ColorEdit4("color:", light.data(), ImGuiColorEditFlags_Float);
          ImGui::Separator();
        }
      }
    }
    if (registry.has<Material>(eid) && ImGui::CollapsingHeader("Material")) {
      auto& material = registry.get<Material>(eid);
      ImGui::ColorEdit3("ambient:", glm::value_ptr(material.ambient));
      ImGui::ColorEdit3("diffuse:", glm::value_ptr(material.diffuse));
      ImGui::ColorEdit3("specular:", glm::value_ptr(material.specular));
      ImGui::SliderFloat("shininess:", &material.shininess, 0.0f, 1.0f);
    }
  };

  imgui_cxx::with_window(draw, "Entity Editor Window");
}

void
draw_time_editor(common::Logger& logger, Time& time, UiDebugState& uistate)
{
  if (ImGui::Begin("TimeWindow")) {
    int speed = 0;
    ImGui::InputInt("Speed Multiplier", &speed);
    ImGui::Separator();

    auto& buffer = uistate.buffers.draw_time_window;
    ImGui::InputInt("Seconds", &buffer.second);
    ImGui::InputInt("Minutes", &buffer.minute);
    ImGui::InputInt("Hour", &buffer.hour);
    ImGui::InputInt("Day", &buffer.day);
    ImGui::InputInt("Week", &buffer.week);
    ImGui::InputInt("Month", &buffer.month);
    ImGui::InputInt("Year", &buffer.year);

    ImGui::Checkbox("Clear Time Fields With Submission", &buffer.clear_fields);

    auto const maybe_clear_fields = [&]() {
      bool const clear_fields = buffer.clear_fields;
      if (clear_fields) {
        common::memzero(&buffer, sizeof(DrawTimeBuffer));
      }

      // restore clear_fields
      buffer.clear_fields = clear_fields;
    };
    if (ImGui::Button("Add Time Offset")) {
      time.add_seconds(buffer.second);
      time.add_minutes(buffer.minute);
      time.add_hours(buffer.hour);
      time.add_days(buffer.day);
      time.add_weeks(buffer.week);
      time.add_months(buffer.month);
      time.add_years(buffer.year);
      maybe_clear_fields();
    }
    ImGui::SameLine();
    if (ImGui::Button("Set Time")) {
      time.reset();

      time.set_seconds(buffer.second);
      time.set_minutes(buffer.minute);
      time.set_hours(buffer.hour);
      time.set_days(buffer.day);
      time.set_weeks(buffer.week);
      time.set_months(buffer.month);
      time.set_years(buffer.year);
      maybe_clear_fields();
    }

    ImGui::TextWrapped("Current Time: Year: %d, Month: %d, Week: %d, Day: %d, Hour: %d, "
                       "Minute: %d, Second: %d",
                       time.years(), time.months(), time.weeks(), time.days(), time.hours(),
                       time.minutes(), time.seconds());
    ImGui::End();
  }
}

void
show_water_window(EngineState& es, LevelManager& lm)
{
  auto& zs      = lm.active();
  auto& ldata   = zs.level_data;
  auto& uistate = es.ui_state.debug;
  auto& wbuffer = uistate.buffers.water;

  auto&      registry = zs.registry;
  auto const winfos   = find_all_entities_with_component<WaterInfo>(registry);

  auto const draw = [&]() {
    ImGui::Text("Water Info");
    ImGui::Separator();
    ImGui::Text("Global Fields");
    ImGui::InputFloat("weight \%light", &wbuffer.weight_light);
    ImGui::InputFloat("weight \%texture", &wbuffer.weight_texture);
    ImGui::InputFloat("weight \%mix_effect", &wbuffer.weight_mix_effect);
    ImGui::Separator();
    ImGui::Text("Edit properties for individual water instances:");
    ImGui::Separator();

    auto pairs = collect_name_eid_pairs<WaterInfo, Transform>(registry);
    auto&      buffer        = uistate.buffers.water.selected_waterinfo;
    display_combo_for_pairs("WaterInfo:", &buffer, pairs);

    if (-1 != buffer) {
      assert(buffer >= 0);
      assert(static_cast<size_t>(buffer) < winfos.size());
      EntityID const weid = pairs[buffer].second;
      auto&      wi   = registry.get<WaterInfo>(weid);
      ImGui::ColorEdit4("Mix Color", wi.mix_color.data());
      ImGui::InputFloat("Mix-Intensity", &wi.mix_intensity);

      ImGui::InputFloat("Wave Offset",   &wi.wave_offset);
      ImGui::InputFloat("Wave Strength (not currently used :( )", &wi.wave_strength);

      auto constexpr WAVE_MIN = -1.0f, WAVE_MAX = 1.0f;
      auto *direction_ptr     = glm::value_ptr(wi.flow_direction);
      if (ImGui::SliderFloat2("Wave Direction", direction_ptr, WAVE_MIN, WAVE_MAX)) {
        wi.flow_direction = glm::normalize(wi.flow_direction);
      }
    }
  };

  imgui_cxx::with_window(draw, "Water Window");
}

void
draw_terrain_editor(EngineState& es, LevelManager& lm)
{
  auto& logger = es.logger;

  auto& zs        = lm.active();
  auto& gfx_state = zs.gfx_state;
  auto& tbuffers  = es.ui_state.debug.buffers.terrain;

  auto& ldata        = zs.level_data;
  auto& terrain_grid = ldata.terrain;

  auto const fn = [&](auto const i, auto const j, auto& buffer) {
    buffer << "(";
    buffer << std::to_string(i);
    buffer << ", ";
    buffer << std::to_string(j);
    buffer << ")";
    buffer << '\0';
  };
  auto const tgrid_slots_string = [&]() {
    std::stringstream buffer;
    visit_each(terrain_grid, fn, buffer);
    buffer << '\0';
    return buffer.str();
  };

  auto& ttable      = gfx_state.texture_table;
  auto& ld          = zs.level_data;
  auto& grid_config = terrain_grid.config;
  auto& sps         = gfx_state.sps;

  auto const draw = [&]() -> Result<common::none_t, std::string> {
    if (ImGui::CollapsingHeader("Regenerate Grid")) {
      auto& tbuffer_gridconfig = tbuffers.grid_config;
      imgui_cxx::input_sizet("num rows", &tbuffer_gridconfig.num_rows);
      imgui_cxx::input_sizet("num cols", &tbuffer_gridconfig.num_cols);
      ImGui::InputFloat("x width", &tbuffer_gridconfig.dimensions.x);
      ImGui::InputFloat("z length", &tbuffer_gridconfig.dimensions.y);

      if (ImGui::Button("Generate Terrain")) {
        auto&      terrain_config = tbuffers.terrain_config;
        auto const heightmap      = TRY_MOVEOUT(
            heightmap::load_fromtable(logger, ttable, terrain_config.texture_names.heightmap_path));

        terrain_grid.config = tbuffer_gridconfig;
        auto& sp            = sps.ref_sp(terrain_config.shader_name);
        ldata.terrain =
            terrain::generate_grid(logger, terrain_config, heightmap, sp, ldata.terrain);
      }
    }
    if (ImGui::CollapsingHeader("Rendering Options")) {
      {
        auto constexpr WINDING_OPTIONS = common::make_array<GLint>(GL_CCW, GL_CW);
        terrain_grid.winding           = imgui_cxx::combo_from_array(
            "Winding Order", "CCW\0CW\0\0", &tbuffers.selected_winding, WINDING_OPTIONS);
      }
      ImGui::Checkbox("Culling Enabled", &terrain_grid.culling_enabled);
      {
        auto constexpr CULLING_OPTIONS =
            common::make_array<GLint>(GL_BACK, GL_FRONT, GL_FRONT_AND_BACK);
        terrain_grid.culling_mode =
            imgui_cxx::combo_from_array("Culling Face", "Front\0Back\0Front And Back\0\0",
                                        &tbuffers.selected_culling, CULLING_OPTIONS);
      }
    }
    if (ImGui::CollapsingHeader("Update Existing Terrain")) {
      auto const tgrid_slot_names = tgrid_slots_string();
      if (ImGui::Combo("Select Terrain", &tbuffers.selected_terrain, tgrid_slot_names.c_str())) {
        tbuffers.terrain_config = terrain_grid[tbuffers.selected_terrain].config;
      }
      ImGui::Separator();

      auto& terrain_config = tbuffers.terrain_config;
      imgui_cxx::input_sizet("Vertex Count", &terrain_config.num_vertexes_along_one_side);
      ImGui::InputFloat("height multiplier", &terrain_config.height_multiplier);
      ImGui::Checkbox("Invert Normals", &terrain_config.invert_normals);
      ImGui::Checkbox("Tile Textures", &terrain_config.tile_textures);
      {
        auto const nicknames = ttable.list_of_all_names('\0') + "\0";

        ImGui::Separator();
        auto& terrain_texturenames = terrain_config.texture_names;
        table_combo("Heightmap", terrain_texturenames.heightmap_path, &tbuffers.selected_heightmap,
                    nicknames, ttable);

        auto const shader_names = sps.all_shader_names_flattened('\0') + "\0";
        table_combo("Shader", terrain_config.shader_name, &tbuffers.selected_shader, shader_names,
                    sps);

        FOR(i, tbuffers.selected_textures.size())
        {
          auto& st = tbuffers.selected_textures[i];

          // clang-format off
          auto constexpr SAMPLER_NAMES = common::make_array<char const*>(
              "u_bgsampler",
              "u_rsampler",
              "u_gsampler",
              "u_bsampler",
              "u_blendsampler");
          // clang-format on
          table_combo(SAMPLER_NAMES[i], terrain_texturenames.textures[i], &st, nicknames, ttable);
        }
      }
      {
        auto constexpr WRAP_OPTIONS =
            common::make_array<GLint>(GL_MIRRORED_REPEAT, GL_REPEAT, GL_CLAMP_TO_EDGE);
        terrain_config.wrap_mode =
            imgui_cxx::combo_from_array("UV Wrap Mode", "Mirrored Repeat\0Repeat\0Clamp\0\0",
                                        &tbuffers.selected_wrapmode, WRAP_OPTIONS);
      }
      ImGui::InputFloat("UV Modifier", &terrain_config.uv_modifier);
      if (ImGui::Button("Regenerate Piece")) {

        auto& terrain_texturenames = terrain_config.texture_names;
        assert(terrain_texturenames.textures.size() > 0);

        auto const update_selectedtexture =
            [&](size_t const index) -> Result<common::Nothing, std::string> {
          int const tbuf_index = tbuffers.selected_textures[index];

          // Lookup texture in the texture table
          auto tn_o = ttable.nickname_at_index(tbuf_index);
          assert(tn_o);
          std::string const tn = *tn_o;

          auto const* p_texture = ttable.lookup_nickname(tn);
          if (!p_texture) {
            auto const fmt = fmt::sprintf("ERROR Looking up texture: %s", tn);
            return Err(fmt);
          }
          else {
            auto* ti = ttable.find(tn);
            assert(ti);
            ti->while_bound(logger, [&]() {
              ti->set_fieldi(GL_TEXTURE_WRAP_S, terrain_config.wrap_mode);
              ti->set_fieldi(GL_TEXTURE_WRAP_T, terrain_config.wrap_mode);
            });
          }
          terrain_texturenames.textures[index] = tn;
          return OK_NONE;
        };

        DO_EFFECT(update_selectedtexture(0));
        auto const selected_hm = ttable.nickname_at_index(tbuffers.selected_heightmap)
                                     .value_or(terrain_texturenames.heightmap_path);

        terrain_config.shader_name =
            sps.nickname_at_index(tbuffers.selected_shader).value_or(terrain_config.shader_name);

        auto const heightmap = TRY_MOVEOUT(heightmap::load_fromtable(logger, ttable, selected_hm));

        auto const selected_terrain = tbuffers.selected_terrain;
        int const  row              = selected_terrain / terrain_grid.num_rows();
        int const  col              = selected_terrain % terrain_grid.num_cols();

        auto& sp = sps.ref_sp(terrain_config.shader_name);
        auto  tp = terrain::generate_piece(logger, glm::vec2{row, col}, grid_config, terrain_config,
                                          heightmap, sp);

        LOG_ERROR_SPRINTF("SELECTED TERRAIN %i row %i col %i", selected_terrain, row, col);
        terrain_grid[selected_terrain] = MOVE(tp);
      }
    }

    return OK_NONE;
  };
  imgui_cxx::with_window_logerrors(logger, draw, "Terrain Editor Window");
}

void
draw_camera_window(Camera& camera, Player& player)
{
  auto const draw_thirdperson_controls = [](CameraArcball const& tp_camera) {
    auto const scoords = tp_camera.spherical_coordinates();
    {
      auto const r      = scoords.radius_display_string();
      auto const t      = scoords.theta_display_string();
      auto const p      = scoords.phi_display_string();
      ImGui::Text("Spherical Coordinates:\tr: '%s', t: '%s', p: '%s'", r.c_str(), t.c_str(),
                  p.c_str());
    }
    {
      auto const text = glm::to_string(tp_camera.world_position());
      ImGui::Text("world position: '%s'\ncamera phi: %f,\tcamera theta: %f", text.c_str(),
          scoords.phi, scoords.theta);
    }
    {
      auto const cart = to_cartesian(scoords);
      ImGui::Text("Cartesian Coordinates\t x: %f, y: %f, z: %f",
                  cart.x, cart.y, cart.z);
    }
    {
      glm::vec3 const tfp = tp_camera.target_position();
      ImGui::Text("Follow Target position\t x: %f, y: %f, z: %f", tfp.x, tfp.y, tfp.z);
    }
  };
  auto const draw_camera_window = [&]() {
    auto& tp_camera = camera.arcball;
    ImGui::Checkbox("Flip Y Sensitivity", &tp_camera.flip_y);
    ImGui::Checkbox("Mouse Rotation Lock", &tp_camera.rotate_lock);
    ImGui::InputFloat("Mouse Rotation Speed", &tp_camera.rotation_speed);
    ImGui::Separator();

    ImGui::Text("Viewport");
    auto& viewport = camera.viewport_ref();
    ImGui::InputFloat("FOV:", &viewport.field_of_view);

    auto& aspect_ratio = viewport.aspect_ratio;
    ImGui::InputFloat2("Aspect:", aspect_ratio.data());
    ImGui::Separator();

    auto& frustum = camera.frustum_ref();
    ImGui::Text("Frustum");
    ImGui::InputFloat("Left:",   &frustum.left);
    ImGui::InputFloat("Right:",  &frustum.right);
    ImGui::InputFloat("Bottom:", &frustum.bottom);
    ImGui::InputFloat("Top:",    &frustum.top);
    ImGui::InputFloat("Far:",    &frustum.far);
    ImGui::InputFloat("Near:",   &frustum.near);
    ImGui::Separator();

    auto mode_strings = CameraModes::string_list();
    int selected = static_cast<int>(camera.mode());
    void* pdata = reinterpret_cast<void*>(&mode_strings);
    if (ImGui::Combo("Mode:", &selected, callback_from_strings, pdata, mode_strings.size())) {
      auto const mode = static_cast<CameraMode>(selected);
      camera.set_mode(mode);
    }

    if (ImGui::CollapsingHeader("Third Person Information")) {
      draw_thirdperson_controls(camera.arcball);
    }
  };
  imgui_cxx::with_window(draw_camera_window, "CAMERA INFO WINDOW");
}

void
draw_mouse_window(MouseState& mstate)
{
  auto const draw = [&]() {
    ImGui::InputFloat("X sensitivity:", &mstate.sensitivity.x, 0.0f, 1.0f);
    ImGui::InputFloat("Y sensitivity:", &mstate.sensitivity.y, 0.0f, 1.0f);
  };
  imgui_cxx::with_window(draw, "MOUSE INFO WINDOW");
}

void
draw_player_window(EngineState& es, Player& player)
{
  auto const draw = [&]() {
    auto wo = player.world_object();
    auto const display = wo.display();
    ImGui::Text("%s", display.c_str());

    ImGui::Checkbox("Player Collisions Enabled", &es.player_collision);
    ImGui::Checkbox("Localspace Vectors Vectors", &es.show_player_localspace_vectors);
    ImGui::Checkbox("Worldspace Vectors Vectors", &es.show_player_worldspace_vectors);
    ImGui::InputFloat("Player Speed", &player.speed);

    glm::quat const quat = glm::angleAxis(glm::radians(0.0f), math::constants::Y_UNIT_VECTOR);
    float const     dot  = glm::dot(wo.orientation(), quat);
    ImGui::Text("dot product: %f", dot);
  };
  imgui_cxx::with_window(draw, "PLAYER INFO WINDOW");
}

void
draw_skybox_window(EngineState& es, LevelManager& lm, SkyboxRenderer& skyboxr)
{
  auto& logger = es.logger;
  auto& zs     = lm.active();
  auto& ldata  = zs.level_data;
  auto& skybox = ldata.skybox;

  auto& gfx_state = zs.gfx_state;
  auto& ttable    = gfx_state.texture_table;
  auto* pti       = ttable.find("night_skybox");
  assert(pti);
  auto& ti = *pti;

  auto const draw = [&]() {
    // auto const draw_button = [&]() {
    // ImTextureID im_texid = reinterpret_cast<void*>(ti.id);

    // imgui_cxx::ImageButtonBuilder image_builder;
    // image_builder.frame_padding = 1;
    // image_builder.bg_color      = ImColor{255, 255, 255, 255};
    // image_builder.tint_color    = ImColor{255, 255, 255, 128};

    // auto const size = ImVec2(32, 32);
    // return image_builder.build(im_texid, size);
    //};
    auto const skybox_combo = [&](char const* text, char const* init, auto* buffer,
                                  auto const& fn) {
      auto const nicknames = ttable.list_of_all_names('\0') + "\0";
      if (table_combo(text, init, buffer, nicknames, ttable)) {
        auto* ti = ttable.find(init);
        assert(ti);
        (skyboxr.*fn)(ti);
      }
    };

    auto& sbuffers = es.ui_state.debug.buffers.skybox;
    skybox_combo("day", "building_skybox", &sbuffers.selected_day, &SkyboxRenderer::set_day);
    skybox_combo("night", "night_skybox", &sbuffers.selected_night, &SkyboxRenderer::set_night);
  };
  imgui_cxx::with_window(draw, "Skybox Window");
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
show_environment_window(UiDebugState& state, LevelData& ldata)
{
  auto&      fog  = ldata.fog;
  auto const draw = [&]() {
    ImGui::Text("Fog");
    ImGui::ColorEdit4("Color:", fog.color.data());

    ImGui::SliderFloat("Density", &fog.density, 0.0f, 0.01f);
    ImGui::SliderFloat("Gradient", &fog.gradient, 0.0f, 10.0f);
    ImGui::Separator();

    ImGui::Text("Wind");
    auto& wind = ldata.wind;
    ImGui::InputFloat("Speed", &wind.speed);

    bool const close_pressed = ImGui::Button("Close", ImVec2(120, 0));
    state.show_environment_window    = !close_pressed;
  };
  imgui_cxx::with_window(draw, "Fog Window");
}

void
world_menu(EngineState& es, LevelData& ldata)
{
  auto&      ui   = es.ui_state.debug;
  auto const draw = [&]() {
    ImGui::MenuItem("Update Orbital", nullptr, &es.update_orbital_bodies);

    ImGui::MenuItem("Local Axis", nullptr, &es.show_local_axis);
    ImGui::MenuItem("Global Axis", nullptr, &es.show_global_axis);
  };
  imgui_cxx::with_menu(draw, "World");
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
draw_mainmenu(EngineState& es, LevelManager& lm, window::SDLWindow& window, DrawState& ds)
{
  auto&      uistate      = es.ui_state.debug;
  auto const windows_menu = [&]() {
    ImGui::MenuItem("Camera", nullptr, &uistate.show_camerawindow);
    ImGui::MenuItem("Entity", nullptr, &uistate.show_entitywindow);
    ImGui::MenuItem("Environment Window", nullptr, &uistate.show_environment_window);
    ImGui::MenuItem("Mouse", nullptr, &uistate.show_mousewindow);
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

namespace boomhs::ui_debug
{

void
draw(EngineState& es, LevelManager& lm, SkyboxRenderer& skyboxr, WaterAudioSystem& water_audio,
     window::SDLWindow& window, Camera& camera, DrawState& ds, window::FrameTime const& ft)
{
  auto& uistate        = es.ui_state.debug;
  auto& zs             = lm.active();
  auto& registry       = zs.registry;
  auto& ldata          = zs.level_data;

  auto& player = find_player(registry);

  if (uistate.show_entitywindow) {
    auto const cstate = CameraFrameState::from_camera(camera);
    FrameState fstate{cstate, es, zs};
    auto const& view_mat = fstate.view_matrix();
    auto const& proj_mat = fstate.projection_matrix();
    draw_entity_editor(es, lm, registry, camera, view_mat, proj_mat);
  }
  if (uistate.show_time_window) {
    draw_time_editor(es.logger, es.time, uistate);
  }
  if (uistate.show_camerawindow) {
    draw_camera_window(camera, player);
  }
  if (uistate.show_mousewindow) {
    draw_mouse_window(es.mouse_states.current);
  }
  if (uistate.show_playerwindow) {
    draw_player_window(es, player);
  }
  if (uistate.show_skyboxwindow) {
    draw_skybox_window(es, lm, skyboxr);
  }
  if (uistate.show_terrain_editor_window) {
    draw_terrain_editor(es, lm);
  }
  if (uistate.show_environment_window) {
    show_environment_window(uistate, ldata);
  }
  if (uistate.show_water_window) {
    show_water_window(es, lm);
  }
  draw_mainmenu(es, lm, window, ds);
}

} // namespace boomhs::ui_debug
