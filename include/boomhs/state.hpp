#pragma once
#include <boomhs/game_config.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/time.hpp>
#include <boomhs/ui_state.hpp>

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

struct EngineState
{
  stlw::Logger&    logger;
  ALCdevice&       al_device;
  ImGuiIO&         imgui;
  Dimensions const dimensions;
  MainMenuState    main_menu;
  Time             time;

  bool                 quit              = false;
  bool                 game_running      = false;
  GameGraphicsSettings graphics_settings = {};

  bool player_collision;
  bool mariolike_edges;
  bool draw_imguimetrics;

  bool update_orbital_bodies = true;

  // rendering state
  bool draw_bounding_boxes;
  bool draw_2d_billboard_entities;
  bool draw_2d_ui_entities;

  bool draw_3d_entities;
  bool draw_fbo_testwindow;
  bool draw_terrain;
  bool draw_normals;
  bool draw_skybox;

  bool show_global_axis;
  bool show_local_axis;

  bool show_player_localspace_vectors;
  bool show_player_worldspace_vectors;

  bool show_grid_lines;
  bool show_yaxis_lines;
  bool wireframe_override;

  MouseState  mouse_state  = {};
  WindowState window_state = {};
  UiState     ui_state     = {};

  // Constructors
  NO_COPY_OR_MOVE(EngineState);
  EngineState(stlw::Logger&, ALCdevice&, ImGuiIO&, Dimensions const&);
};

struct GameState
{
  EngineState& engine_state;
  LevelManager level_manager;

  NO_COPY(GameState);
  NO_MOVE_ASSIGN(GameState);
  GameState(GameState&&);
  // MOVE_CONSTRUCTIBLE_ONLY(GameState);

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
  NO_COPY_OR_MOVE(Engine);

  Dimensions dimensions() const;
  ScreenSize screen_size() const;
};

} // namespace boomhs
