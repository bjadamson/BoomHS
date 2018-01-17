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
#include <boomhs/world_object.hpp>
#include <boomhs/tilemap.hpp>
#include <vector>

using stlw::Logger;

namespace boomhs
{

// other
static constexpr auto INIT_ATTENUATION_INDEX = 8;

struct LightColors
{
  opengl::Color ambient = LOC::WHITE;
  opengl::Color diffuse = LOC::WHITE;
  opengl::Color specular = LOC::WHITE;

  opengl::Attenuation attenuation = opengl::ATTENUATION_VALUE_TABLE[INIT_ATTENUATION_INDEX];

  // TODO: this is a hack, should be based on other entities position(s)
  glm::vec3 single_light_position{0.0f, 0.0f, 0.0f};
};

struct MaterialColors
{
  opengl::Color ambient{1.0f, 1.0f, 1.0f, 1.0f};
  opengl::Color diffuse{1.0f, 1.0f, 1.0f, 1.0f};
  opengl::Color specular{1.0f, 1.0f, 1.0f, 1.0f};
  float shininess = 32.0f;
};

struct RenderArgs
{
  Logger &logger;
  Camera const& camera;
  WorldObject const& player;

  std::vector<Transform*> &entities;
  LightColors &light;
  MaterialColors &at_materials;
};

struct UiState
{
  bool enter_pressed = false;
  bool block_input = false;
  bool rotate_lock = false;
  bool flip_y = false;

  bool show_lighting_window = false;


  // primitive buffers
  int eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
  glm::vec3 last_mouse_clicked_pos;
  int attenuation_current_item = INIT_ATTENUATION_INDEX;
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

struct TilemapRender
{
  bool redraw = true;
  bool reveal = false;

  // Both related to drawing GRID LINES
  bool show_grid_lines = false;
  bool show_yaxis_lines = false;
};

struct RenderState
{
  bool draw_entities = true;
  bool draw_skybox = false;
  bool draw_tilemap = true;

  bool show_global_axis = true;
  bool show_local_axis = false;
  bool show_target_vectors = true;

  opengl::Color background = LOC::BLACK;

  TilemapRender tilemap;
};

struct Collision
{
  bool player = true;
};

struct GameState
{
  bool quit = false;

  Collision collision;
  MouseState mouse;
  RenderState render;
  WindowState window;

  // singular light in the scene
  LightColors light;

  // Materials for the "@" (player)
  MaterialColors at_materials;

  Logger &logger;
  ImGuiIO &imgui;
  window::Dimensions const dimensions;
  stlw::float_generator rnum_generator;

  TileMap tilemap;
  window::mouse_data mouse_data;

  UiState ui_state;

  // NOTE: Keep this data member above the "camera" data member.
  std::vector<Transform*> entities;

  Camera camera;
  WorldObject player;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  GameState(Logger &l, ImGuiIO &i, window::Dimensions const &d, stlw::float_generator &&fg,
      TileMap &&tmap, std::vector<Transform*> &&ents, Camera &&cam, WorldObject &&pl)
    : logger(l)
    , imgui(i)
    , dimensions(d)
    , rnum_generator(MOVE(fg))
    , tilemap(MOVE(tmap))
    , mouse_data(window::make_default_mouse_data())
    , entities(MOVE(ents))
    , camera(MOVE(cam))
    , player(MOVE(pl))
  {
  }

  RenderArgs render_args()
  {
    return RenderArgs{this->logger, this->camera, this->player, this->entities, this->light,
      this->at_materials};
  }
};

} // ns boomhs
