#include <boomhs/io.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/zone.hpp>

#include <window/controller.hpp>
#include <window/timer.hpp>

#include <stlw/math.hpp>
#include <stlw/log.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <iostream>

float constexpr MOVE_DISTANCE = 1.0f;
float constexpr SCALE_FACTOR = 0.20f;
float constexpr ZOOM_FACTOR = 0.2f;

namespace {
using namespace boomhs;
using namespace window;

void
move_ontilegrid(GameState &state, glm::vec3 (WorldObject::*fn)() const, WorldObject &wo,
    FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &ts = es.tilegrid_state;

  ZoneManager zm{state.zone_states};
  LevelData const& leveldata = zm.active().level_state.level_data;
  auto const [x, y] = leveldata.dimensions();
  glm::vec3 const move_vec = (wo.*fn)();

  // TODO: stop doing this when we use double instead of float
  auto const dtf = static_cast<float>(ft.delta);

  glm::vec2 const wpos = wo.tile_position() + (move_vec * dtf * wo.speed());

  bool const x_outofbounds = wpos.x > x || wpos.x < 0;
  bool const y_outofbounds = wpos.y > y || wpos.y < 0;
  bool const out_of_bounds = x_outofbounds || y_outofbounds;
  if (out_of_bounds && es.mariolike_edges) {
    if (x_outofbounds) {
      auto const new_x = wpos.x < 0 ? x : 0;
      wo.move_to(new_x, 0.0, wpos.y);
    }
    else if (y_outofbounds) {
      auto const new_y = wpos.y < 0 ? y : 0;
      wo.move_to(wpos.x, 0.0, new_y);
    }
  } else if (out_of_bounds) {
    return;
  }
  auto const tpos = TilePosition::from_floats_truncated(wpos.x, wpos.y);
  bool const should_move = (!es.player_collision) || !leveldata.is_wall(tpos);
  if (should_move) {
    wo.move(move_vec, ft.delta);
    ts.recompute = true;
  }
}

bool
is_quit_event(SDL_Event &event)
{
  bool is_quit = false;

  switch (event.type) {
  case SDL_QUIT: {
    is_quit = true;
    break;
  }
  case SDL_KEYDOWN: {
    switch (event.key.keysym.sym) {
    case SDLK_ESCAPE: {
      is_quit = true;
      break;
    }
    }
  }
  }
  return is_quit;
}

void
process_mousemotion(GameState &state, SDL_MouseMotionEvent const& motion, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &ms = es.mouse_state;
  auto &ts = es.tilegrid_state;
  auto &ui = es.ui_state;

  ZoneManager zm{state.zone_states};
  auto &lstate = zm.active().level_state;
  auto &player = lstate.player;
  auto &camera = lstate.camera;

  auto const xrel = motion.xrel;
  auto const yrel = motion.yrel;

  if (ms.both_pressed()) {
    player.rotate_to_match_camera_rotation(camera);
    move_ontilegrid(state, &WorldObject::world_forward, player, ft);
  }
  if (ms.left_pressed) {
    auto const& sens = ms.sensitivity;
    float const dx = sens.x * xrel;
    float const dy = sens.y * yrel;
    camera.rotate(dx, dy);
  }
  if (ms.right_pressed) {
    float const speed = camera.rotation_speed;
    float const angle = xrel > 0 ? speed : -speed;

    auto const x_dt = angle * ft.delta;
    auto constexpr y_dt = 0.0f;
    player.rotate(x_dt, opengl::Y_UNIT_VECTOR);
  }
}

void
process_mousebutton_down(GameState &state, SDL_MouseButtonEvent const& event, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &ms = es.mouse_state;

  auto const& button = event.button;
  if (button == SDL_BUTTON_LEFT) {
    ms.left_pressed = true;
  }
  else if (button == SDL_BUTTON_RIGHT) {
    ms.right_pressed = true;
  }
  if (ms.both_pressed()) {
    LOG_ERROR("toggling mouse up/down (pitch) lock");
    ms.pitch_lock ^= true;

    ZoneManager zm{state.zone_states};
    auto &lstate = zm.active().level_state;
    auto &player = lstate.player;
    auto &camera = lstate.camera;

    player.rotate_to_match_camera_rotation(camera);
  }
}

void
process_mousebutton_up(GameState &state, SDL_MouseButtonEvent const& event, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &ms = es.mouse_state;

  auto const& button = event.button;
  if (SDL_BUTTON_LEFT == button) {
    ms.left_pressed = false;
  }
  else if (SDL_BUTTON_RIGHT == button) {
    ms.right_pressed = false;
  }
}

void
process_keyup(GameState &state, SDL_Event const& event, FrameTime const& ft)
{
}

void
process_keydown(GameState &state, SDL_Event const& event, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &ui = es.ui_state;
  auto &ts = es.tilegrid_state;

  ZoneManager zm{state.zone_states};
  auto &lstate= zm.active().level_state;
  auto &player = lstate.player;
  auto const rotate_player = [&](float const angle, glm::vec3 const& axis) {
    player.rotate(angle, axis);
    ts.recompute = true;
  };
  switch (event.key.keysym.sym) {
    case SDLK_F11:
      ui.draw_ui ^= true;
      break;
    case SDLK_t:
      // invert
      //state.camera.toggle_mode();
      break;
    // scaling
    case SDLK_KP_PLUS:
    case SDLK_o:
      //et.scale_entities(sf(SCALE_FACTOR));
      break;
    case SDLK_KP_MINUS:
    /*
    case SDLK_p:
      {
        // 1) Convert mouse location to
        // 3d normalised device coordinates
        int const mouse_x = es.mouse_data.current.x, mouse_y = es.mouse_data.current.y;
        auto const width = es.dimensions.w, height = es.dimensions.h;
        float const x = (2.0f * mouse_x) / width - 1.0f;
        float const y = 1.0f - (2.0f * mouse_y) / height;
        float const z = 1.0f;
        glm::vec3 const ray_nds{x, y, z};

        // homongonize coordinates
        glm::vec4 const ray_clip{ray_nds.x, ray_nds.y, -1.0, 1.0};

        //auto &a = camera.perspective_ref();
        //glm::vec4 ray_eye = perspectiveInverse(a.field_of_view, a.viewport_aspect_ratio, a.near_plane, a.far_plane) * ray_clip;
        //ray_eye.z = -1.0f;
        //ray_eye.w = 0.0f;

        //glm::vec3 const ray_wor = glm::normalize(glm::vec3{glm::inverse(camera.view_matrix()) * ray_eye});
        //std::cerr << "mouse: '" << std::to_string(mouse_x) << "', '" << std::to_string(mouse_y) << "'\n";
        //std::cerr << "ray_wor: '" << ray_wor << "'\n";

        //glm::vec3 const ray_dir = ray_eye;
        //glm::vec3 const ray_origin = camera.world_position();
        //glm::vec3 const plane_origin{0, 0, 0};
        //glm::vec3 const plane_normal{0, -1, 0};

        //float distance = 0.0f;
        //bool intersects = glm::intersectRayPlane(ray_origin, ray_dir, plane_origin, plane_normal, distance);
        //std::cerr << "intersects: '" << intersects << "', distance: '" << distance << "'\n";

        auto const ray = calculate_mouse_worldpos(camera, player, mouse_x, mouse_y, es.dimensions);
      }
      break;
    */
    // z-rotation
      break;
    case SDLK_j: {
      auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};
      //et.rotate_entities(ANGLE, ROTATION_VECTOR);
      break;
    }
    case SDLK_k: {
      auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};
      //et.rotate_entities(-ANGLE, ROTATION_VECTOR);
      break;
    }
    // y-rotation
    case SDLK_u: {
      auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
      //et.rotate_entities(ANGLE, ROTATION_VECTOR);
      break;
    }
    case SDLK_i: {
      auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
      //et.rotate_entities(-ANGLE, ROTATION_VECTOR);
      break;
    }
    // x-rotation
    case SDLK_n: {
      auto constexpr ROTATION_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
      //et.rotate_entities(ANGLE, ROTATION_VECTOR);
      break;
    }
    case SDLK_m: {
      auto constexpr ROTATION_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
      //et.rotate_entities(-ANGLE, ROTATION_VECTOR);
      break;
    }
    case SDLK_RETURN:
      // Toggle state
      ui.enter_pressed ^= true;
      break;
    case SDLK_LEFT:
      rotate_player(90.0f, opengl::Y_UNIT_VECTOR);
      break;
    case SDLK_RIGHT:
      rotate_player(-90.0f, opengl::Y_UNIT_VECTOR);
      break;
  }
}

void
process_mousewheel(GameState &state, SDL_MouseWheelEvent const& wheel, FrameTime const& ft)
{
  auto &logger = state.engine_state.logger;
  LOG_TRACE("mouse wheel event detected.");

  ZoneManager zm{state.zone_states};
  auto &lstate = zm.active().level_state;
  auto &camera = lstate.camera;
  if (wheel.y > 0) {
    camera.decrease_zoom(ZOOM_FACTOR);
  } else {
    camera.increase_zoom(ZOOM_FACTOR);
  }
}

void
process_mousestate(GameState &state, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &ms = es.mouse_state;
  if (ms.both_pressed()) {

    ZoneManager zm{state.zone_states};
    auto &lstate = zm.active().level_state;
    auto &player = lstate.player;

    move_ontilegrid(state, &WorldObject::world_forward, player, ft);
  }
}

void
process_keystate(GameState &state, FrameTime const& ft)
{
  // continual keypress responses procesed here
  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);

  auto &es = state.engine_state;
  auto &ts = es.tilegrid_state;
  ZoneManager zm{state.zone_states};
  auto &lstate = zm.active().level_state;
  auto &player = lstate.player;

  if (keystate[SDL_SCANCODE_W]) {
    move_ontilegrid(state, &WorldObject::world_forward, player, ft);
  }
  if (keystate[SDL_SCANCODE_S]) {
    move_ontilegrid(state, &WorldObject::world_backward, player, ft);
  }
  if (keystate[SDL_SCANCODE_A]) {
    move_ontilegrid(state, &WorldObject::world_left, player, ft);
  }
  if (keystate[SDL_SCANCODE_D]) {
    move_ontilegrid(state, &WorldObject::world_right, player, ft);
  }
  if (keystate[SDL_SCANCODE_Q]) {
    move_ontilegrid(state, &WorldObject::world_up, player, ft);
  }
  if (keystate[SDL_SCANCODE_E]) {
    move_ontilegrid(state, &WorldObject::world_down, player, ft);
  }
}

void
process_controllerstate(GameState &state, SDLControllers const& controllers, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &c = controllers.first();

  SDL_Joystick *joystick = c.joystick;
  assert(joystick);

  auto const read_axis = [&c](auto const axis) {
    return SDL_GameControllerGetAxis(c.controller.get(), axis);
  };

  // https://wiki.libsdl.org/SDL_GameControllerGetAxis
  //
  // using 32bit ints to be sure no overflow (maybe unnecessary?)
  int32_t constexpr AXIS_MIN = -32768;
  int32_t constexpr AXIS_MAX = 32767;

  ZoneManager zm{state.zone_states};
  auto &lstate = zm.active().level_state;
  auto &camera = lstate.camera;
  auto &player = lstate.player;

  auto constexpr THRESHOLD = 0.4f;
  auto const less_threshold = [](auto const& v) {
    return v <= 0 && (v <= AXIS_MIN * THRESHOLD);
  };
  auto const greater_threshold = [](auto const& v) {
    return v >= 0 && (v >= AXIS_MAX * THRESHOLD);
  };

  auto const left_axis_x = c.left_axis_x();
  if (less_threshold(left_axis_x)) {
    player.rotate_to_match_camera_rotation(camera);
    move_ontilegrid(state, &WorldObject::world_left, player, ft);
  }
  if (greater_threshold(left_axis_x)) {
    player.rotate_to_match_camera_rotation(camera);
    move_ontilegrid(state, &WorldObject::world_right, player, ft);
  }
  auto const left_axis_y = c.left_axis_y();
  if (less_threshold(left_axis_y)) {
    player.rotate_to_match_camera_rotation(camera);
    move_ontilegrid(state, &WorldObject::world_forward, player, ft);
  }
  if (greater_threshold(left_axis_y)) {
    player.rotate_to_match_camera_rotation(camera);
    move_ontilegrid(state, &WorldObject::world_backward, player, ft);
  }

  //auto const& controller_sensitivity = ???;
  auto constexpr CONTROLLER_SENSITIVITY = 0.01;

  auto const calc_delta = [&ft](auto const axis) {
    return axis * ft.delta * 0.01;
  };
  {
    auto const right_axis_x = c.right_axis_x();
    if (less_threshold(right_axis_x)) {
      float const dx = calc_delta(right_axis_x);
      camera.rotate(dx, 0.0);
    }
    if (greater_threshold(right_axis_x)) {
      float const dx = calc_delta(right_axis_x);
      camera.rotate(dx, 0.0);
    }
  }
  {
    auto const right_axis_y = c.right_axis_y();
    if (less_threshold(right_axis_y)) {
      float const dy = calc_delta(right_axis_y);
      camera.rotate(0.0, dy);
    }
    if (greater_threshold(right_axis_y)) {
      float const dy = calc_delta(right_axis_y);
      camera.rotate(0.0, dy);
    }
  }
}

bool
process_event(GameState &state, SDL_Event &event, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;

  // If the user pressed enter, don't process mouse events (for the game)
  auto &ui = es.ui_state;
  if (ui.block_input || ui.enter_pressed) {
    return is_quit_event(event);
  }

  switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
      process_mousebutton_down(state, event.button, ft);
      break;
    case SDL_MOUSEBUTTONUP:
      process_mousebutton_up(state, event.button, ft);
      break;
    case SDL_MOUSEMOTION:
      process_mousemotion(state, event.motion, ft);
      break;
    case SDL_MOUSEWHEEL:
      process_mousewheel(state, event.wheel, ft);
      break;
    case SDL_KEYDOWN:
      process_keydown(state, event, ft);
      break;
    case SDL_KEYUP:
      process_keyup(state, event, ft);
      break;
  }
  return is_quit_event(event);
}

} // ns anon

namespace boomhs
{

void
IO::process(GameState &state, SDL_Event &event, SDLControllers const& controllers,
    FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;

  LOG_TRACE("IO::process(data, state)");
  while ((!es.quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);

    auto &imgui = es.imgui;
    if (!imgui.WantCaptureMouse && !imgui.WantCaptureKeyboard) {
      es.quit = process_event(state, event, ft);
    }
  }
  process_mousestate(state, ft);
  process_keystate(state, ft);
  process_controllerstate(state, controllers, ft);
}

} // ns boomhs
