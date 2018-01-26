#pragma once
#include <boomhs/assets.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/shader.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.hpp>
#include <vector>

using stlw::Logger;

namespace boomhs
{

struct UiState
{
  bool enter_pressed = false;
  bool block_input = false;
  bool rotate_lock = false;
  bool flip_y = false;

  bool show_background_window = false;

  int selected_pointlight = 0;
  bool show_pointlight_window = false;

  bool show_ambientlight_window = false;
  bool show_directionallight_window = false;

  int selected_material = 0;
  bool show_entitymaterial_window = false;

  int selected_entity = 0;
  int selected_level = 0;

  // primitive buffers
  int eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
  glm::vec3 last_mouse_clicked_pos;
  int attenuation_current_item = opengl::Light::INIT_ATTENUATION_INDEX;

  bool show_debugwindow = true;
  bool show_entitywindow = false;
  bool show_camerawindow = false;
  bool show_playerwindow = false;
  bool show_tilemapwindow = false;
};

struct MouseState
{
  bool left_pressed = false;
  bool right_pressed = false;
};

struct WindowState
{
  window::FullscreenFlags fullscreen = window::NOT_FULLSCREEN;
};

struct TilemapState
{
  bool draw_tilemap = true;
  bool recompute = true;
  bool reveal = false;

  // Both related to drawing GRID LINES
  bool show_grid_lines = true;
  bool show_yaxis_lines = false;

  glm::vec3 floor_offset = {0.0f, -0.5f, 0.0f};
  glm::vec3 tile_scaling = {0.5f, 0.5f, 0.5f};
};

struct ZoneState
{
  // singular light in the scene
  opengl::Color background;
  opengl::GlobalLight global_light;

  HandleManager handles;
  opengl::ShaderPrograms sps;
  TileMap tilemap;

  Camera camera;
  WorldObject player;
  entt::DefaultRegistry &registry;

  explicit ZoneState(opengl::Color const& bgcolor, opengl::GlobalLight const& glight,
      HandleManager &&hm, opengl::ShaderPrograms &&sp, TileMap &&tmap, Camera &&cam,
      WorldObject &&pl, entt::DefaultRegistry &reg)
    : background(bgcolor)
    , global_light(MOVE(glight))
    , handles(MOVE(hm))
    , sps(MOVE(sp))
    , tilemap(MOVE(tmap))
    , camera(MOVE(cam))
    , player(MOVE(pl))
    , registry(reg)
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(ZoneState);
};

class ZoneStates
{
  // This class is meant to be used through the ZoneManager class, construct an instance of
  // ZoneManager to work with this data.
public:
  static std::size_t constexpr NUM_ZSTATES = 2;
private:
  std::array<ZoneState, NUM_ZSTATES> zstates_;
  int active_ = 0;
public:
  MOVE_CONSTRUCTIBLE_ONLY(ZoneStates);

  // TODO: pass in active zone to support loading levels
  explicit ZoneStates(std::array<ZoneState, NUM_ZSTATES> &&zstates)
    : zstates_(MOVE(zstates))
  {
  }
private:
  friend class ZoneManager;
  auto const&
  data() const
  {
    return zstates_;
  }

  auto&
  data()
  {
    return zstates_;
  }

  auto
  active() const
  {
    return active_;
  }

  void
  set_active(int const zone_number)
  {
    active_ = zone_number;
  }

  auto
  size() const
  {
    return zstates_.size();
  }
};

struct EngineState
{
  bool quit = false;
  bool player_collision = false;

  // rendering state
  bool draw_entities = true;
  bool draw_skybox = false;
  bool draw_terrain = false;
  bool draw_normals = false;

  bool show_global_axis = true;
  bool show_local_axis = false;
  bool show_target_vectors = true;

  MouseState mouse_state;
  WindowState window_state;
  TilemapState tilemap_state;

  Logger &logger;
  ImGuiIO &imgui;
  window::Dimensions const dimensions;

  window::mouse_data mouse_data;
  UiState ui_state;

  // Constructors
  MOVE_CONSTRUCTIBLE_ONLY(EngineState);
  EngineState(Logger &l, ImGuiIO &i, window::Dimensions const &d)
    : logger(l)
    , imgui(i)
    , dimensions(d)
    , mouse_data(window::make_default_mouse_data())
  {
  }
};

struct GameState
{
  EngineState engine_state;
  ZoneStates zone_states;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  explicit GameState(EngineState &&es, ZoneStates &&zs)
    : engine_state(MOVE(es))
    , zone_states(MOVE(zs))
  {
  }

  RenderArgs
  render_args();
};

} // ns boomhs
