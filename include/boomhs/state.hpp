#pragma once
#include <boomhs/level_manager.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/time.hpp>
#include <boomhs/ui_state.hpp>
#include <boomhs/zone_state.hpp>

#include <window/controller.hpp>
#include <window/mouse.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/openal.hpp>

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

  bool draw_tilegrid = false;
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
  stlw::Logger&    logger;
  ALCdevice&       al_device;
  ImGuiIO&         imgui;
  Dimensions const dimensions;
  MainMenuState    main_menu;
  Time             time;

  bool quit         = false;
  bool game_running = false;

  bool player_collision;
  bool mariolike_edges;
  bool draw_imguimetrics;

  // rendering state
  bool advanced_water;
  bool draw_bounding_boxes;
  bool draw_entities;
  bool draw_fbo_testwindow;
  bool draw_terrain;
  bool draw_water;
  bool draw_normals;
  bool draw_skybox;

  bool show_global_axis;
  bool show_local_axis;

  bool show_player_localspace_vectors;
  bool show_player_worldspace_vectors;
  bool wireframe_override = false;

  MouseState    mouse_state    = {};
  WindowState   window_state   = {};
  TiledataState tilegrid_state = {};
  UiState       ui_state       = {};

  // Constructors
  MOVE_CONSTRUCTIBLE_ONLY(EngineState);
  EngineState(stlw::Logger&, ALCdevice&, ImGuiIO&, Dimensions const&);
};

struct GameState
{
  EngineState& engine_state;
  LevelManager level_manager;

  MOVE_CONSTRUCTIBLE_ONLY(GameState);

  explicit GameState(EngineState&, LevelManager&&);
};

using EntityRegistries = std::vector<EntityRegistry>;

struct Engine
{
  window::SDLWindow      window;
  window::SDLControllers controllers;

  EntityRegistries registries = {};

  Engine() = delete;
  explicit Engine(window::SDLWindow&&, window::SDLControllers&&);

  // We mark this as no-move/copy so the registries data never moves, allowing the rest of the
  // program to store references into the data owned by registries.
  NO_COPYMOVE(Engine);

  Dimensions dimensions() const;
  ScreenSize screen_size() const;
};

} // namespace boomhs
