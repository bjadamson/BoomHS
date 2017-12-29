#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>

#include <boomhs/camera.hpp>
#include <boomhs/player.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/skybox.hpp>
#include <vector>

using stlw::Logger;

namespace boomhs
{

struct UiState
{
  bool enter_pressed = false;
  bool block_input = false;
  bool flip_y = false;
  bool show_global_axis = true;
  bool show_local_axis = false;
  bool show_target_vectors = true;

  // primitive buffers
  int eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
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

struct RenderState
{
  bool draw_skybox = false;
  bool redraw_tilemap = true;
};

struct GameState
{
  bool quit = false;

  MouseState mouse;
  RenderState render;
  WindowState window;

  Logger &logger;
  ImGuiIO &imgui;
  window::Dimensions const dimensions;
  stlw::float_generator rnum_generator;

  TileMap tilemap;
  window::mouse_data mouse_data;

  UiState ui_state;

  // NOTE: Keep this data member above the "camera" data member.
  std::vector<Transform*> entities;

  // player needs to come AFTER "camera".
  Camera camera;
  Player player;
  Skybox skybox;

  static constexpr std::size_t COLOR_CUBE_INDEX = 0;
  static constexpr std::size_t TEXTURE_CUBE_INDEX = 1;
  static constexpr std::size_t WIREFRAME_CUBE_INDEX = 2;
  static constexpr std::size_t SKYBOX_INDEX = 3;
  static constexpr std::size_t HOUSE_INDEX = 4;
  static constexpr std::size_t AT_INDEX = 5;
  static constexpr std::size_t PLAYER_ARROW_INDEX = 6;
  static constexpr std::size_t TILEMAP_INDEX = 7;
  static constexpr std::size_t TERRAIN_INDEX = 8;
  static constexpr std::size_t CAMERA_INDEX = 9;
  static constexpr std::size_t GLOBAL_AXIS_X_INDEX = 10;
  static constexpr std::size_t GLOBAL_AXIS_Y_INDEX = 11;
  static constexpr std::size_t GLOBAL_AXIS_Z_INDEX = 12;

  static constexpr std::size_t LOCAL_AXIS_X_INDEX = 13;
  static constexpr std::size_t LOCAL_AXIS_Y_INDEX = 14;
  static constexpr std::size_t LOCAL_AXIS_Z_INDEX = 15;

  static constexpr std::size_t LOCAL_FORWARD_INDEX = 16;
  static constexpr std::size_t CAMERA_ARROW_INDEX0 = 17;
  static constexpr std::size_t CAMERA_ARROW_INDEX1 = 18;
  static constexpr std::size_t CAMERA_ARROW_INDEX2 = 19;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  GameState(Logger &l, ImGuiIO &i, window::Dimensions const &d, stlw::float_generator &&fg,
      TileMap &&tmap, std::vector<Transform*> &&ents, Camera &&cam, Player &&pl, Skybox &&sbox)
    : logger(l)
    , imgui(i)
    , dimensions(d)
    , rnum_generator(MOVE(fg))
    , tilemap(MOVE(tmap))
    , mouse_data(window::make_default_mouse_data())
    , entities(MOVE(ents))
    , camera(MOVE(cam))
    , player(MOVE(pl))
    , skybox(MOVE(sbox))
  {
    //this->camera.move_down(1);
  }

  RenderArgs render_args() const
  {
    return RenderArgs{this->logger, this->camera};
  }
};

} // ns boomhs
