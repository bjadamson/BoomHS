#pragma once
#include <boomhs/level_manager.hpp>
#include <boomhs/time.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/zone_state.hpp>

#include <window/controller.hpp>
#include <window/mouse.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

struct ImGuiIO;
namespace boomhs
{

struct MouseState
{
  MOVE_CONSTRUCTIBLE_ONLY(MouseState);

  bool left_pressed  = false;
  bool right_pressed = false;

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

  window::FullscreenFlags  fullscreen = window::FullscreenFlags::NOT_FULLSCREEN;
  window::SwapIntervalFlag sync       = window::SwapIntervalFlag::SYNCHRONIZED;
};

struct TiledataState
{
  MOVE_CONSTRUCTIBLE_ONLY(TiledataState);

  bool draw_tilegrid = true;
  bool recompute     = true;
  bool reveal        = false;

  // Both related to drawing GRID LINES
  bool show_grid_lines          = false;
  bool show_yaxis_lines         = false;
  bool show_neighbortile_arrows = false;

  glm::vec3 floor_offset = {0.0f, -0.5f, 0.0f};
  glm::vec3 tile_scaling = {1.0f, 1.0f, 1.0f};
};

struct EngineState
{
  stlw::Logger&            logger;
  ImGuiIO&                 imgui;
  window::Dimensions const dimensions;
  Time                     time;

  bool quit           = false;
  bool game_running   = false;
  bool show_main_menu = true;

  bool player_collision;
  bool mariolike_edges;
  bool draw_imguimetrics;

  // rendering state
  bool draw_entities;
  bool draw_terrain;
  bool draw_normals;
  bool draw_sun;

  bool show_global_axis;
  bool show_local_axis;

  bool show_player_localspace_vectors;
  bool show_player_worldspace_vectors;

  MouseState    mouse_state    = {};
  WindowState   window_state   = {};
  TiledataState tilegrid_state = {};
  UiState       ui_state       = {};

  // Constructors
  MOVE_CONSTRUCTIBLE_ONLY(EngineState);
  EngineState(stlw::Logger&, ImGuiIO&, window::Dimensions const&);
};

struct GameState
{
  EngineState  &engine_state;
  LevelManager level_manager;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);

  explicit GameState(EngineState&, LevelManager&&);
};

struct Engine
{
  window::SDLWindow           window;
  window::SDLControllers      controllers;
  std::vector<EntityRegistry> registries = {};

  Engine() = delete;
  explicit Engine(window::SDLWindow&&, window::SDLControllers&&);

  // We mark this as no-move/copy so the registries data never moves, allowing the rest of the
  // program to store references into the data owned by registries.
  NO_COPYMOVE(Engine);

  auto dimensions() const { return window.get_dimensions(); }
};

} // namespace boomhs
