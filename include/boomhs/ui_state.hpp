#pragma once
#include <boomhs/terrain.hpp>

#include <array>
#include <stlw/math.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs
{

struct UiInGameState
{
};

struct DrawTimeBuffer
{
  int second = 0;
  int minute = 0;
  int hour   = 0;
  int day    = 0;
  int week   = 0;
  int month  = 0;
  int year   = 0;

  bool clear_fields = false;
};

struct TerrainBuffer
{
  int selected_winding  = 0;
  int selected_culling  = 0;
  int selected_wrapmode = 0;
  int selected_terrain  = 0;

  TerrainPieceConfig config;
};

struct Buffers
{
  DrawTimeBuffer draw_time_window;
  TerrainBuffer  terrain;
};

struct UiDebugState
{
  MOVE_CONSTRUCTIBLE_ONLY(UiDebugState);

  bool enter_pressed         = false;
  bool block_input           = false;
  bool update_orbital_bodies = true;

  // Buffers
  Buffers buffers;

  int selected_entity          = 0;
  int selected_entity_material = 0;

  int selected_pointlight = 0;

  std::array<int, 2> selected_tile     = {0};
  int                selected_tilegrid = 0;

  // primitive buffers
  int       eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
  glm::vec3 last_mouse_clicked_pos;

  // window display state
  bool show_ambientlight_window     = false;
  bool show_camerawindow            = false;
  bool show_debugwindow             = true;
  bool show_directionallight_window = false;

  bool show_entitymaterial_window = false;
  bool show_entitywindow          = false;

  bool show_fog_window  = false;
  bool show_mousewindow = false;

  bool show_playerwindow      = false;
  bool show_pointlight_window = false;
  bool show_skyboxwindow      = false;
  bool show_time_window       = false;

  bool show_tilegrid_editor_window  = false;
  bool show_tilegridmaterial_window = false;
  bool show_terrain_editor_window   = false;
};

struct UiState
{
  UiDebugState  debug;
  UiInGameState ingame;

  bool draw_ingame_ui = true;
  bool draw_debug_ui  = true;
};

} // namespace boomhs
