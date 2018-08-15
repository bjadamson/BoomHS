#pragma once
#include <boomhs/game_config.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/time.hpp>
#include <boomhs/ui_state.hpp>

#include <window/controller.hpp>
#include <window/sdl_window.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <extlibs/openal.hpp>

struct ImGuiIO;
namespace boomhs
{

class MouseState
{
  auto mask() const { return SDL_GetMouseState(nullptr, nullptr); }

public:
  MouseSensitivity sensitivity{0.002f, 0.002f};

  auto coords() const
  {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return ScreenCoordinates{x, y};
  }
  bool left_pressed() const { return mask() & SDL_BUTTON(SDL_BUTTON_LEFT); }
  bool right_pressed() const { return mask() & SDL_BUTTON(SDL_BUTTON_RIGHT); }
  bool middle_pressed() const { return mask() & SDL_BUTTON(SDL_BUTTON_MIDDLE); }

  bool both_pressed() const { return left_pressed() && right_pressed(); }
  bool either_pressed() const { return left_pressed() || right_pressed(); }
};

struct MouseStates
{
  MouseState current;
  MouseState previous;
};

struct WindowState
{
  MOVE_CONSTRUCTIBLE_ONLY(WindowState);

  window::FullscreenFlags  fullscreen = window::FullscreenFlags::NOT_FULLSCREEN;
  window::SwapIntervalFlag sync       = window::SwapIntervalFlag::SYNCHRONIZED;
};

struct EngineState
{
  common::Logger&        logger;
  ALCdevice&             al_device;
  ImGuiIO&               imgui;
  ScreenDimensions const dimensions;
  MainMenuState          main_menu;
  Time                   time;

  bool                 quit                  = false;
  bool                 game_running          = false;
  bool                 update_orbital_bodies = true;
  GameGraphicsSettings graphics_settings     = {};

  MouseStates mouse_states = {};
  WindowState window_state = {};
  UiState     ui_state     = {};

  bool disable_controller_input;
  bool player_collision;
  bool mariolike_edges;
  bool draw_imguimetrics;

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

  // Constructors
  NO_COPY_OR_MOVE(EngineState);
  EngineState(common::Logger&, ALCdevice&, ImGuiIO&, ScreenDimensions const&);
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
  NO_COPY_OR_MOVE(Engine);

  ScreenDimensions dimensions() const;
  ScreenSize       screen_size() const;
};

} // namespace boomhs
