#pragma once
#include <boomhs/terrain.hpp>
#include <boomhs/ui_ingame.hpp>

#include <boomhs/math.hpp>
#include <common/type_macros.hpp>

#include <array>

namespace boomhs
{

struct UiInGameState
{
  ChatBuffer  chat_buffer;
  ChatHistory chat_history;
  ChatState   chat_state;
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

  bool clear_fields = true;
};

struct SkyboxBuffer
{
  int selected_day   = -1;
  int selected_night = -1;
};

struct TerrainBuffer
{
  int selected_winding  = 0;
  int selected_culling  = 0;
  int selected_wrapmode = 0;
  int selected_terrain  = 0;

  int selected_heightmap = -1;

  std::array<int, 5> selected_textures = {-1, -1, -1, -1, -1};
  int                selected_shader   = -1;

  TerrainConfig     terrain_config;
  TerrainGridConfig grid_config;
};

struct AudioUiBuffer
{
  float ambient = 0.227f;
};

struct WaterBuffer
{
  bool draw                        = true;
  int  selected_water_graphicsmode = 0;
  int  selected_waterinfo          = -1;

  float weight_light      = 0.60f;
  float weight_texture    = 1.0f;
  float weight_mix_effect = 1.0f;
};

struct LogBuffer
{
  int log_level = 2;
};

struct Buffers
{
  AudioUiBuffer  audio;
  DrawTimeBuffer draw_time_window;
  LogBuffer      log;
  SkyboxBuffer   skybox;
  TerrainBuffer  terrain;
  WaterBuffer    water;
};

struct UiDebugState
{
  MOVE_CONSTRUCTIBLE_ONLY(UiDebugState);

  // Buffers
  Buffers buffers;

  // primitive buffers
  int       eid_buffer = 0;
  glm::vec3 euler_angle_buffer;
  glm::vec3 last_mouse_clicked_pos;

  // window display state
  bool lock_debugselected           = false;
  bool show_ambientlight_window     = false;
  bool show_camerawindow            = false;
  bool show_debugwindow             = true;
  bool show_directionallight_window = false;

  bool show_entitywindow = false;

  bool show_environment_window = false;
  bool show_devicewindow       = false;

  bool show_playerwindow = false;
  bool show_skyboxwindow = false;
  bool show_time_window  = false;

  bool show_terrain_editor_window = false;
  bool show_water_window          = false;
};

struct UiState
{
  UiDebugState  debug;
  UiInGameState ingame;

  bool draw_ingame_ui = true;
  bool draw_debug_ui  = true;
};

} // namespace boomhs
