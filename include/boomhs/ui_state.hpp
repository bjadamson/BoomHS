#pragma once
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>
#include <array>

namespace boomhs
{

struct UiInGameState
{
};

struct UiDebugState
{
  MOVE_CONSTRUCTIBLE_ONLY(UiDebugState);

  bool enter_pressed = false;
  bool block_input = false;

  int selected_entity = 0;
  int selected_entity_material = 0;

  int selected_pointlight = 0;

  std::array<int, 2> selected_tile = {0};
  int selected_tilegrid = 0;

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

struct UiState
{
  UiDebugState debug;
  UiInGameState ingame;

  bool draw_ingame_ui = true;
  bool draw_debug_ui = true;
};

} // ns boomhs
