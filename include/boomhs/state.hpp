#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <opengl/camera.hpp>
#include <opengl/renderer.hpp>
#include <window/mouse.hpp>

#include <boomhs/player.hpp>
#include <boomhs/tilemap.hpp>
#include <vector>

using stlw::Logger;

namespace boomhs
{

struct UiState
{
  bool enter_pressed = false;
  bool block_input = false;
  bool flip_y = false;

  // primitive buffers
  int eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
};

struct GameState
{
  bool quit = false;
  bool draw_skybox = false;
  Logger &logger;
  ImGuiIO &imgui;
  window::Dimensions const dimensions;
  stlw::float_generator rnum_generator;

  TileMap tilemap;
  window::mouse_data mouse_data;

  UiState ui_state;

  // NOTE: Keep this data member above the "camera" data member.
  std::vector<::opengl::Model*> entities;

  // player needs to come AFTER "camera".
  opengl::Camera camera;
  Player player;

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
  static constexpr std::size_t AXIS_X_INDEX = 10;
  static constexpr std::size_t AXIS_Y_INDEX = 11;
  static constexpr std::size_t AXIS_Z_INDEX = 12;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  GameState(Logger &l, ImGuiIO &i, window::Dimensions const &d, stlw::float_generator &&fg,
      TileMap &&tmap, std::vector<::opengl::Model*> &&ents, opengl::Camera &&cam, Player &&pl)
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
    //this->camera.move_down(1);
  }

  opengl::RenderArgs render_args() const
  {
    return opengl::RenderArgs{this->logger, this->camera};
  }
};

} // ns boomhs
