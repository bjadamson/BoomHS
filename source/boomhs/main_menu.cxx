#include <boomhs/audio.hpp>
#include <boomhs/bounding_object.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/color.hpp>
#include <boomhs/components.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/math.hpp>
#include <boomhs/player.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/ui_debug.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/view_frustum.hpp>
#include <boomhs/water.hpp>
#include <boomhs/zone_state.hpp>

#include <opengl/renderer.hpp>
#include <opengl/global.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox_renderer.hpp>

#include <common/time.hpp>

#include <gl_sdl/sdl_window.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/imgui.hpp>
#include <extlibs/sdl.hpp>

#include <algorithm>
#include <optional>
#include <sstream>

using namespace boomhs;
using namespace opengl;
using namespace gl_sdl;

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
draw_menu(EngineState& es, Viewport const& dimensions, WaterAudioSystem& water_audio)
{
  auto const size      = ImVec2(dimensions.right(), dimensions.bottom());
  //ImVec2 const size{dimensions.right(), dimensions.bottom()};
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

  ImGui::Separator();
  ImGui::Text("Test GRID");
  ImGui::Checkbox("Show", &es.grid_lines.show);
  ImGui::InputFloat3("Dimensions", glm::value_ptr(es.grid_lines.dimensions));

  ImGui::Separator();
  ImGui::Checkbox("ImGui Metrics", &es.draw_imguimetrics);
  if (es.draw_imguimetrics) {
    ImGui::ShowMetricsWindow(&es.draw_imguimetrics);
  }
}

void
process_keydown(GameState& state, SDL_Event const& event)
{
  auto& es = state.engine_state();
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
log_menu(EngineState& es, LevelData& ldata)
{
  auto& logger  = es.logger;
  auto& ui      = es.ui_state.debug;
  auto& buffers = ui.buffers;

  bool constexpr yes = true;
  auto const log_menu_item = [&](char const* name, auto const level) {
    auto const level_int = static_cast<int>(ui.buffers.log.log_level);
    bool const* selected = (level == level_int) ? &yes : nullptr;
    if (ImGui::MenuItem(name, nullptr, selected)) {
      logger.set_level(level);
      ui.buffers.log.log_level = level_int;
    }
  };

  log_menu_item("Trace", spdlog::level::trace);
  log_menu_item("Debug", spdlog::level::debug);
  log_menu_item("Info",  spdlog::level::info);
  log_menu_item("Warn",  spdlog::level::warn);
  log_menu_item("Error", spdlog::level::err);
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
draw_time_editor(common::Logger& logger, common::Time& time, UiDebugState& uistate)
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
    ImGui::Separator();
    ImGui::Text("Advanced Water Options");
    ImGui::InputFloat("u_fresnel_reflect_power", &wbuffer.fresnel_reflect_power);
    ImGui::InputFloat("u_depth_divider", &wbuffer.depth_divider);
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
draw_camera_window(Camera& camera, Player& player, Frustum& frustum)
{
  auto const draw_thirdperson_controls = [](CameraArcball const& tp_camera) {
    auto const scoords = tp_camera.spherical_coordinates();
    {
      auto const r = scoords.radius;
      auto const t = glm::degrees(scoords.theta);
      auto const p = glm::degrees(scoords.phi);
      ImGui::Text("Spherical Coordinates:\tr: '%f', t: '%f', p: '%f'", r, t, p);
    }
    {
      auto const c = to_cartesian(scoords);
      ImGui::Text("Cartesian Coordinates\t%s", glm::to_string(c).c_str());
    }
    {
      auto const text = glm::to_string(tp_camera.world_position());
      ImGui::Text("Camera world position :\t%s", text.c_str());
    }
    {
      auto const tp = glm::to_string(tp_camera.target_position());
      ImGui::Text("Follow Target position:\t%s", tp.c_str());
    }
  };
  auto const draw_window = [&]() {
    {
      auto mode_strings = CameraModes::string_list();
      int selected = static_cast<int>(camera.mode());
      void* pdata = reinterpret_cast<void*>(&mode_strings);
      if (ImGui::Combo("Mode:", &selected, callback_from_strings, pdata, mode_strings.size())) {
        auto const mode = static_cast<CameraMode>(selected);
        camera.set_mode(mode);
      }
      ImGui::Separator();
    }
    {
      ImGui::Text("View Settings");
      auto& view_settings = camera.view_settings_ref();

      auto fov = glm::degrees(view_settings.field_of_view);
      if (ImGui::InputFloat("FOV (degrees):", &fov)) {
        view_settings.field_of_view = glm::radians(fov);
      }

      auto& aspect_ratio = view_settings.aspect_ratio;
      ImGui::InputFloat2("Aspect:", aspect_ratio.data());
    }
    if (ImGui::CollapsingHeader("Frustum")) {
      ImGui::InputInt("Left:",     &frustum.left);
      ImGui::InputInt("Right:",    &frustum.right);
      ImGui::InputInt("Bottom:",   &frustum.bottom);
      ImGui::InputInt("Top:",      &frustum.top);

      ImGui::InputFloat("Far:",    &frustum.far);
      ImGui::InputFloat("Near:",   &frustum.near);
    }
    if (ImGui::CollapsingHeader("FPS Camera")) {
      auto const rot = glm::degrees(glm::eulerAngles(camera.get_target().orientation()));
      ImGui::Text("FollowTarget Rotation: %s", glm::to_string(rot).c_str());
    }
    if (ImGui::CollapsingHeader("Arcball Camera")) {
      draw_thirdperson_controls(camera.arcball);
    }
    if (ImGui::CollapsingHeader("Ortho Camera")) {
      auto& ortho = camera.ortho;
      ImGui::InputFloat3("Position", glm::value_ptr(ortho.position));
    }
  };
  imgui_cxx::with_window(draw_window, "CAMERA INFO WINDOW");
}

void
draw_device_window(DeviceStates& dstates, Camera& camera)
{
  auto const draw = [&]() {
    ImGui::Separator();

    auto& fps     = camera.fps.cs;
    auto& arcball = camera.arcball.cs;
    if (ImGui::CollapsingHeader("Mouse")) {
      {
        ImGui::Text("First Person");
        ImGui::InputFloat("MOUSE FPS X sensitivity:", &fps.sensitivity.x, 0.0f, 1.0f);
        ImGui::InputFloat("MOUSE FPS Y sensitivity:", &fps.sensitivity.y, 0.0f, 1.0f);
        ImGui::Checkbox("MOUSE FPS Invert X", &fps.flip_x);
        ImGui::Checkbox("MOUSE FPS Invert Y", &fps.flip_y);

        ImGui::Checkbox("MOUSE FPS Rotation Lock", &fps.rotation_lock);
      }
      ImGui::Separator();
      {
        ImGui::Text("Third Person");
        ImGui::InputFloat("MOUSE TPS X sensitivity:",  &arcball.sensitivity.x, 0.0f, 1.0f);
        ImGui::InputFloat("MOUSE TPS Y sensitivity:",  &arcball.sensitivity.y, 0.0f, 1.0f);
        ImGui::Checkbox("MOUSE TPS Invert X", &arcball.flip_x);
        ImGui::Checkbox("MOUSE TPS Invert Y", &arcball.flip_y);

        ImGui::Checkbox("MOUSE TPS Rotation Lock", &arcball.rotation_lock);
      }
    }
    if (ImGui::CollapsingHeader("Controller")) {
      auto& sens = dstates.controller;
      {
        ImGui::Text("First Person");
        ImGui::InputFloat("Controler FPS X sensitivity:", &fps.sensitivity.x, 0.0f, 1.0f);
        ImGui::InputFloat("Controler FPS Y sensitivity:", &fps.sensitivity.y, 0.0f, 1.0f);
        ImGui::Checkbox("Controler FPS Rotation Lock", &fps.rotation_lock);
      }
      ImGui::Separator();
      {
        ImGui::InputFloat("Controler TPS X sensitivity:",  &arcball.sensitivity.x, 0.0f, 1.0f);
        ImGui::InputFloat("Controler TPS Y sensitivity:",  &arcball.sensitivity.y, 0.0f, 1.0f);
        ImGui::Checkbox("Controler TPS Rotation Lock", &arcball.rotation_lock);
      }
    }
  };
  imgui_cxx::with_window(draw, "Device Info Window");
}

void
draw_player_window(EngineState& es, Player& player)
{
  auto const draw = [&]() {
    auto &wo = player.world_object();
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
draw_mainmenu(EngineState& es, LevelManager& lm, SDLWindow& window, DrawState& ds)
{
  auto&      uistate      = es.ui_state.debug;
  auto&      zs            = lm.active();
  auto&      ldata         = zs.level_data;

  auto const windows_menu = [&]() {
    ImGui::MenuItem("Camera", nullptr, &uistate.show_camerawindow);
    ImGui::MenuItem("Entity", nullptr, &uistate.show_entitywindow);
    ImGui::MenuItem("Device", nullptr, &uistate.show_devicewindow);
    ImGui::MenuItem("Environment Window", nullptr, &uistate.show_environment_window);
    imgui_cxx::with_menu(log_menu, "Log", es, ldata);
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
    setwindow_row("NOT Fullscreen", FullscreenFlags::NOT_FULLSCREEN);
    setwindow_row("Fullscreen", FullscreenFlags::FULLSCREEN);
    setwindow_row("Fullscreen DESKTOP", FullscreenFlags::FULLSCREEN_DESKTOP);
    auto const setsync_row = [&](char const* text, auto const sync) {
      if (ImGui::MenuItem(text, nullptr, nullptr, window_state.sync != sync)) {
        window.set_swapinterval(sync);
        window_state.sync = sync;
      }
    };
    setsync_row("Synchronized", SwapIntervalFlag::SYNCHRONIZED);
    setsync_row("Late Tearing", SwapIntervalFlag::LATE_TEARING);
  };

  auto&      registry      = zs.registry;
  auto const draw_mainmenu = [&]() {
    imgui_cxx::with_menu(windows_menu, "Windows");
    imgui_cxx::with_menu(settings_menu, "Settings");
    world_menu(es, ldata);
    lighting_menu(es, ldata, registry);
  };
  imgui_cxx::with_mainmenubar(draw_mainmenu);
}


} // namespace

namespace boomhs::main_menu
{

void
draw(EngineState& es, SDLWindow& window, Camera& camera, SkyboxRenderer& skyboxr, DrawState& ds,
     LevelManager& lm, Viewport const& dimensions, WaterAudioSystem& water_audio)
{
  draw_menu(es, dimensions, water_audio);

  auto& uistate = es.ui_state.debug;
  if (uistate.show_debugwindow) {
    auto& zs = lm.active();
    draw_debugwindow(es, zs);
  }

  auto& zs             = lm.active();
  auto& registry       = zs.registry;
  auto& ldata          = zs.level_data;

  auto& player = find_player(registry);
  if (uistate.show_time_window) {
    draw_time_editor(es.logger, es.time, uistate);
  }
  if (uistate.show_camerawindow) {
    draw_camera_window(camera, player, es.frustum);
  }
  if (uistate.show_devicewindow) {
    draw_device_window(es.device_states, camera);
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
