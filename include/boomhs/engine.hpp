#pragma once
#include <boomhs/game_config.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/player.hpp>
#include <boomhs/time.hpp>
#include <boomhs/ui_state.hpp>

#include <window/controller.hpp>
#include <window/sdl_window.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

struct ALCdevice_struct;
struct ImGuiIO;

namespace boomhs
{

struct EngineState
{
  common::Logger&        logger;
  ALCdevice_struct&      al_device;
  ImGuiIO&               imgui;
  ScreenDimensions const dimensions;
  MainMenuState          main_menu;
  Time                   time;

  bool                 quit                  = false;
  bool                 game_running          = false;
  bool                 update_orbital_bodies = true;
  GameGraphicsSettings graphics_settings     = {};

  MouseStates         mouse_states = {};
  window::WindowState window_state = {};
  UiState             ui_state     = {};

  // Current player movement vector
  MovementState movement_state = {};

  bool disable_controller_input;
  bool player_collision;
  bool mariolike_edges;
  bool draw_imguimetrics;

  // rendering state
  bool draw_bounding_boxes;
  bool draw_view_frustum;
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
  EngineState(common::Logger&, ALCdevice_struct&, ImGuiIO&, ScreenDimensions const&);
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
