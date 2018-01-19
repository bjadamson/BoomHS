#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/world_object.hpp>
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

  bool show_globallight_window = false;

  int selected_material = 0;
  bool show_entitymaterial_window = false;

  // primitive buffers
  int eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
  glm::vec3 last_mouse_clicked_pos;
  int attenuation_current_item = opengl::Light::INIT_ATTENUATION_INDEX;
  int entity_window_current = 0;
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
  bool redraw = true;
  bool reveal = false;

  // Both related to drawing GRID LINES
  bool show_grid_lines = false;
  bool show_yaxis_lines = false;
};

struct ZoneState
{
  //PointLights point_lights;

  // singular light in the scene
  opengl::GlobalLight global_light;
  opengl::Color background;

  TileMap tilemap;

  explicit ZoneState(opengl::Color const& bgcolor, TileMap &&tmap)
    : background(bgcolor)
    , tilemap(MOVE(tmap))
  {
  }
};

struct EngineState
{
  bool quit = false;
  bool player_collision = true;

  // rendering state
  bool draw_entities = true;
  bool draw_skybox = false;
  bool draw_tilemap = true;

  bool show_global_axis = true;
  bool show_local_axis = false;
  bool show_target_vectors = true;

  MouseState mouse_state;
  WindowState window_state;
  TilemapState tilemap_state;

  Logger &logger;
  ImGuiIO &imgui;
  window::Dimensions const dimensions;
  stlw::float_generator rnum_generator;

  window::mouse_data mouse_data;
  UiState ui_state;

  Camera camera;
  WorldObject player;

  // Constructors
  MOVE_CONSTRUCTIBLE_ONLY(EngineState);
  EngineState(Logger &l, ImGuiIO &i, window::Dimensions const &d, stlw::float_generator &&fg,
      Camera &&cam, WorldObject &&pl)
    : logger(l)
    , imgui(i)
    , dimensions(d)
    , rnum_generator(MOVE(fg))
    , mouse_data(window::make_default_mouse_data())
    , camera(MOVE(cam))
    , player(MOVE(pl))
  {
  }
};

struct GameState
{
  EngineState engine_state;
  ZoneState zone_state;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  explicit GameState(EngineState &&es, ZoneState &&zs)
    : engine_state(MOVE(es))
    , zone_state(MOVE(zs))
  {
  }

  RenderArgs
  render_args()
  {
    auto &logger = engine_state.logger;

    auto const& camera = engine_state.camera;
    auto const& player = engine_state.player;

    auto const& global_light = zone_state.global_light;

    return RenderArgs{camera, player, logger, global_light};
  }
};

} // ns boomhs
