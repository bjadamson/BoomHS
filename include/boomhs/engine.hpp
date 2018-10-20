#pragma once
#include <boomhs/controller.hpp>
#include <boomhs/game_config.hpp>
#include <boomhs/io_behavior.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/math.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/viewport.hpp>
#include <boomhs/ui_state.hpp>

#include <gl_sdl/sdl_window.hpp>

#include <common/log.hpp>
#include <common/time.hpp>
#include <common/type_macros.hpp>

struct ALCdevice_struct;
struct ImGuiIO;

namespace boomhs
{
struct PlayerBehavior;

struct DeviceStates
{
  ControllerStates controller;
  MouseStates      mouse;
  CursorManager    cursors;
};

struct MovementState
{
  glm::vec3 forward, backward, left, right;

  glm::vec3 mouse_forward;

  NO_COPY_OR_MOVE(MovementState);
};

struct EngineState
{
  common::Logger&   logger;
  ALCdevice_struct& al_device;
  ImGuiIO&          imgui;

  Frustum         frustum;
  MainMenuState   main_menu;
  common::Time    time;
  PlayerBehaviors behaviors;

  bool                 quit                  = false;
  bool                 game_running          = false;
  bool                 update_orbital_bodies = true;
  GameGraphicsSettings graphics_settings     = {};

  DeviceStates        device_states = {};
  gl_sdl::WindowState window_state  = {};
  UiState             ui_state      = {};

  // Current player movement vector
  MovementState movement_state = {};

  // While the user has "pressed" a mouse button down and held it, this value holds the origin of
  // where the user began holding down the mouse button.
  //
  // This field is useful for drawing things relative to where a user clicks and holds down/drags
  // the mouse.
  //
  // ie: Unit selection from a top-down perspective.
  MouseClickPositions mouse_click_origin;

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

  bool show_player_localspace_vectors;
  bool show_player_worldspace_vectors;

  struct GridLinesInfo
  {
    bool      show = false;
    glm::vec3 dimensions = VEC3{10, 1, 10};
  };

  GridLinesInfo grid_lines;

  // Draw *Everything* with a wireframe drawing mode.
  bool wireframe_override;

  // Constructors
  NO_COPY_OR_MOVE(EngineState);
  EngineState(common::Logger&, ALCdevice_struct&, ImGuiIO&, Frustum const& frustum);
};

using EntityRegistries = std::vector<EntityRegistry>;

struct Engine
{
  gl_sdl::SDLWindow window;
  SDLControllers    controllers;

  EntityRegistries registries = {};

  Engine() = delete;
  explicit Engine(gl_sdl::SDLWindow&&, SDLControllers&&);

  // We mark this as no-move/copy so the registries data never moves, allowing the rest of the
  // program to store references into the data owned by registries.
  NO_COPY_OR_MOVE(Engine);

  Viewport window_viewport() const;
};

} // namespace boomhs
