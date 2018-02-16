#pragma once
#include <boomhs/camera.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/lighting.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>

#include <entt/entt.hpp>
#include <imgui/imgui.hpp>
#include <cassert>
#include <vector>

using stlw::Logger;

namespace boomhs
{

struct UiState
{
  MOVE_CONSTRUCTIBLE_ONLY(UiState);

  bool draw_ui = true;
  bool enter_pressed = false;
  bool block_input = false;

  int selected_entity = 0;
  int selected_entity_material = 0;

  int selected_pointlight = 0;

  std::array<int, 2> selected_tile = {0};
  int selected_tilegrid = 0;

  int attenuation_current_item = opengl::Light::INIT_ATTENUATION_INDEX;

  // primitive buffers
  int eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
  glm::vec3 last_mouse_clicked_pos;

  // window display state
  bool show_ambientlight_window = false;
  bool show_background_window = false;
  bool show_camerawindow = false;

  bool show_debugwindow = true;
  bool show_directionallight_window = false;

  bool show_entitywindow = false;
  bool show_entitymaterial_window = false;

  bool show_tilegrid_editor_window = false;
  bool show_tilegridmaterial_window = false;

  bool show_mousewindow = false;
  bool show_playerwindow = false;
  bool show_pointlight_window = false;
};

struct MouseState
{
  MOVE_CONSTRUCTIBLE_ONLY(MouseState);

  bool left_pressed = false;
  bool right_pressed = false;
  bool pitch_lock = true;

  window::MouseSensitivity sensitivity{0.002f, 0.002f};

  // "coords" is meant to updated with the *current* screen coordinates of the mouse.
  // "pcoords" is meant to lag behind one frame from *current*, important for calculating the
  // relative difference per-frame.
  window::ScreenCoordinates coords{0, 0};
  window::ScreenCoordinates relative{0, 0};

  bool both_pressed() const { return left_pressed && right_pressed; }
};

struct WindowState
{
  MOVE_CONSTRUCTIBLE_ONLY(WindowState);

  window::FullscreenFlags fullscreen = window::FullscreenFlags::NOT_FULLSCREEN;
  window::SwapIntervalFlag sync = window::SwapIntervalFlag::SYNCHRONIZED;
};

struct TiledataState
{
  MOVE_CONSTRUCTIBLE_ONLY(TiledataState);

  bool draw_tilegrid = true;
  bool recompute = true;
  bool reveal = false;

  // Both related to drawing GRID LINES
  bool show_grid_lines = true;
  bool show_yaxis_lines = false;
  bool show_neighbortile_arrows = false;

  glm::vec3 floor_offset = {0.0f, -0.5f, 0.0f};
  glm::vec3 tile_scaling = {1.0f, 1.0f, 1.0f};
};

struct GpuState
{
  // These slots get a value when memory is loaded, set to none when memory is not.
  std::optional<opengl::EntityDrawHandles> entities;
  std::optional<opengl::TileDrawHandles> tiles;

  MOVE_CONSTRUCTIBLE_ONLY(GpuState);
};
struct GfxState
{
  GpuState gpu_state = {};
  opengl::ShaderPrograms sps;
  opengl::TextureTable texture_table;

  explicit GfxState(opengl::ShaderPrograms &&sp, opengl::TextureTable &&tt)
    : sps(MOVE(sp))
    , texture_table(MOVE(tt))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(GfxState);
};
struct LevelState
{
  // singular light in the scene
  opengl::Color background;
  opengl::GlobalLight global_light;

  ObjCache obj_cache;
  LevelData level_data;

  Camera camera;
  WorldObject player;
  explicit LevelState(opengl::Color const& bgcolor, opengl::GlobalLight const& glight,
      ObjCache &&ocache, LevelData &&ldata, Camera &&cam, WorldObject &&pl)
    : background(bgcolor)
    , global_light(glight)
    , obj_cache(MOVE(ocache))
    , level_data(MOVE(ldata))
    , camera(MOVE(cam))
    , player(MOVE(pl))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(LevelState);
};
struct ZoneState
{
  LevelState level_state;
  GfxState gfx_state;
  entt::DefaultRegistry &registry;

  explicit ZoneState(LevelState &&level, GfxState &&gfx, entt::DefaultRegistry &reg)
    : level_state(MOVE(level))
    , gfx_state(MOVE(gfx))
    , registry(reg)
  {
  }
  MOVE_CONSTRUCTIBLE_ONLY(ZoneState);
};

// This class is meant to be used through the ZoneManager class, construct an instance of
// ZoneManager to work with this data.
//
// TODO: pass in active zone to support loading levels
class ZoneStates
{
  std::vector<ZoneState> zstates_;
  int active_ = 0;
public:
  MOVE_CONSTRUCTIBLE_ONLY(ZoneStates);
  explicit ZoneStates(std::vector<ZoneState> &&zs)
    : zstates_(MOVE(zs))
  {
  }

  ZoneState& operator[](size_t);
  int size() const;

private:
  friend class ZoneManager;

  int active_zone() const;

  ZoneState& active();
  ZoneState const& active() const;

  //void add_zone(ZoneState &&);
  void set_active(int const);
};

struct EngineState
{
  bool quit = false;
  bool player_collision = false;
  bool mariolike_edges = false;

  // rendering state
  bool draw_entities = true;
  bool draw_skybox = false;
  bool draw_terrain = false;
  bool draw_normals = false;

  bool show_global_axis = true;
  bool show_local_axis = false;

  bool show_player_localspace_vectors = true;
  bool show_player_worldspace_vectors = true;

  MouseState mouse_state = {};
  WindowState window_state = {};
  TiledataState tilegrid_state = {};
  UiState ui_state = {};

  Logger &logger;
  ImGuiIO &imgui;
  window::Dimensions const dimensions;

  // Constructors
  MOVE_CONSTRUCTIBLE_ONLY(EngineState);
  EngineState(Logger &l, ImGuiIO &i, window::Dimensions const &d)
    : logger(l)
    , imgui(i)
    , dimensions(d)
  {
  }
};

struct GameState
{
  EngineState engine_state;
  ZoneStates zone_states;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  explicit GameState(EngineState &&, ZoneStates &&);
};

} // ns boomhs
