#include <boomhs/io.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/level_manager.hpp>

#include <window/controller.hpp>
#include <window/timer.hpp>

#include <stlw/math.hpp>
#include <stlw/log.hpp>

#include <extlibs/imgui.hpp>
#include <iostream>

float constexpr MOVE_DISTANCE = 1.0f;
float constexpr SCALE_FACTOR = 0.20f;
float constexpr ZOOM_FACTOR = 0.2f;

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace {

void
move_ontilegrid(GameState &state, glm::vec3 (WorldObject::*fn)() const, WorldObject &wo,
    FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &ts = es.tilegrid_state;

  auto &lm = state.level_manager;
  LevelData const& leveldata = lm.active().level_data;
  auto const [x, z] = leveldata.dimensions();
  glm::vec3 const move_vec = (wo.*fn)();

  glm::vec3 const delta = move_vec * ft.delta_millis() * wo.speed();
  glm::vec3 const newpos = wo.world_position() + delta;

  bool const x_outofbounds = newpos.x >= x || newpos.x < 0;
  bool const y_outofbounds = newpos.z >= z || newpos.z < 0;
  bool const out_of_bounds = x_outofbounds || y_outofbounds;
  if (out_of_bounds && !es.mariolike_edges) {
    // If the world object *would* be out of bounds, return early (don't move the WO).
    return;
  }

  auto const flip_sides = [](auto const val, auto const min, auto const max) {
    assert(min < (max - 1));
    auto value = val < min ? max : min;
    return value >= max ? (value - 1) : value;
  };

  if (x_outofbounds) {
    auto const new_x = flip_sides(newpos.x, 0ul, x);
    wo.move_to(new_x, 0.0, newpos.z);
    ts.recompute = true;
  }
  else if (y_outofbounds) {
    auto const new_z = flip_sides(newpos.z, 0ul, z);
    wo.move_to(newpos.x, 0.0, new_z);
    ts.recompute = true;
  }
  else {
    auto const tpos = TilePosition::from_floats_truncated(newpos.x, newpos.z);
    bool const should_move = (!es.player_collision) || !leveldata.is_wall(tpos);
    if (should_move) {
      wo.move(delta);
      ts.recompute = true;
    }
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
move_forward(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;

  move_ontilegrid(state, &WorldObject::world_forward, player, ft);
}

void
move_backward(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;
  move_ontilegrid(state, &WorldObject::world_backward, player, ft);
}

void
move_left(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;
  move_ontilegrid(state, &WorldObject::world_left, player, ft);
}

void
move_right(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;
  move_ontilegrid(state, &WorldObject::world_right, player, ft);
}

void
move_up(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;
  move_ontilegrid(state, &WorldObject::world_up, player, ft);
}

void
move_down(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;
  move_ontilegrid(state, &WorldObject::world_down, player, ft);
}

void
try_pickup_nearby_item(GameState &state, FrameTime const& ft)
{
  auto &lm = state.level_manager;
  auto &active = lm.active();
  auto &registry = active.registry;

  auto const player_eid = find_player(registry);
  auto &player_transform = registry.get<Transform>(player_eid);
  auto const& player_pos = player_transform.translation;
  auto &inventory = registry.get<PlayerData>(player_eid).inventory;

  static constexpr auto MINIMUM_DISTANCE_TO_PICKUP = 1.0f;
  auto const items = find_items(registry);
  for(EntityID const eid : items) {
    Item &item = registry.get<Item>(eid);
    if (item.is_pickedup) {
      std::cerr << "item already picked up.\n";
      continue;
    }

    auto &item_transform = registry.get<Transform>(eid);
    auto const& item_pos = item_transform.translation;
    auto const distance = glm::distance(item_pos, player_pos);

    if (distance > MINIMUM_DISTANCE_TO_PICKUP) {
      std::cerr << "There is nothing nearby to pickup.\n";
      continue;
    }

    Player::add_item(eid, item, registry);

    if (registry.has<Torch>(eid)) {
      auto &pointlight = registry.get<PointLight>(eid);
      pointlight.attenuation /= 3.0f;

      std::cerr << "You have picked up a torch.\n";
    }
    else {
      std::cerr << "You have picked up an item.\n";
    }
  }
}

void
process_mousemotion(GameState &state, SDL_MouseMotionEvent const& motion, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &ms = es.mouse_state;
  auto &ts = es.tilegrid_state;
  auto &ui = es.ui_state.debug;

  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;
  auto &camera = ldata.camera;

  auto const xrel = motion.xrel;
  auto const yrel = motion.yrel;

  if (ms.both_pressed()) {
    player.rotate_to_match_camera_rotation(camera);
    move_forward(state, ft);
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

    auto const x_dt = angle * ft.delta_millis();
    auto constexpr y_dt = 0.0f;
    player.rotate_degrees(x_dt, opengl::Y_UNIT_VECTOR);
  }
}

void
process_mousebutton_down(GameState &state, SDL_MouseButtonEvent const& event, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &ms = es.mouse_state;

  auto &zs = state.level_manager.active();
  auto &camera = zs.level_data.camera;

  auto const& button = event.button;
  if (button == SDL_BUTTON_LEFT) {
    ms.left_pressed = true;
  }
  else if (button == SDL_BUTTON_RIGHT) {
    ms.right_pressed = true;
  }
  if (ms.both_pressed()) {
    LOG_ERROR("toggling mouse up/down (pitch) lock");
    camera.rotate_lock ^= true;

    auto &lm = state.level_manager;
    auto &ldata = lm.active().level_data;
    auto &player = ldata.player;
    auto &camera = ldata.camera;

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

  auto &lm = state.level_manager;
  auto &active = lm.active();
  auto &ldata= active.level_data;
  auto &camera = ldata.camera;
  auto &player = ldata.player;
  auto &nearby_targets = ldata.nearby_targets;

  auto const rotate_player = [&](float const angle, glm::vec3 const& axis) {
    player.rotate_degrees(angle, axis);
    ts.recompute = true;
  };
  switch (event.key.keysym.sym) {
    case SDLK_w:
      move_forward(state, ft);
      break;
    case SDLK_s:
      move_backward(state, ft);
      break;
    case SDLK_d:
      move_right(state, ft);
      break;
    case SDLK_a:
      move_left(state, ft);
      break;
    case SDLK_e:
      try_pickup_nearby_item(state, ft);
      //move_up(state, ft);
      break;
    case SDLK_q:
      //move_down(state, ft);
      break;

    case SDLK_F11:
      ui.draw_debug_ui ^= true;
      break;
    case SDLK_t:
      // invert
      camera.next_mode();
      break;
    case SDLK_TAB:
      {
        uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
        assert(keystate);

        if (!keystate[SDL_SCANCODE_LSHIFT]) {
          nearby_targets.cycle_forward();
        }
        else {
          nearby_targets.cycle_backward();
        }
      }
      break;
    case SDLK_BACKQUOTE:
      {
        auto &registry = active.registry;
        auto const eid = find_player(registry);
        auto &inventory = registry.get<PlayerData>(eid).inventory;
        inventory.toggle_open();
      }
      break;
    case SDLK_SPACE:
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

  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &camera = ldata.camera;
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

    auto &lm = state.level_manager;
    auto &ldata = lm.active().level_data;
    auto &player = ldata.player;

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
  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &player = ldata.player;

  if (keystate[SDL_SCANCODE_W]) {
    //move_forward(state, ft);
  }
  if (keystate[SDL_SCANCODE_S]) {
    //move_backward(state, ft);
  }
  if (keystate[SDL_SCANCODE_A]) {
    //move_left(state, ft);
  }
  if (keystate[SDL_SCANCODE_D]) {
    //move_right(state, ft);
  }
  if (keystate[SDL_SCANCODE_Q]) {
    //move_up(state, ft);
  }
  if (keystate[SDL_SCANCODE_E]) {
    //move_down(state, ft);
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

  auto &lm = state.level_manager;
  auto &ldata = lm.active().level_data;
  auto &camera = ldata.camera;
  auto &player = ldata.player;

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
    move_left(state, ft);
  }
  if (greater_threshold(left_axis_x)) {
    player.rotate_to_match_camera_rotation(camera);
    move_right(state, ft);
  }
  auto const left_axis_y = c.left_axis_y();
  if (less_threshold(left_axis_y)) {
    player.rotate_to_match_camera_rotation(camera);
    move_forward(state, ft);
  }
  if (greater_threshold(left_axis_y)) {
    player.rotate_to_match_camera_rotation(camera);
    move_backward(state, ft);
  }

  auto constexpr CONTROLLER_SENSITIVITY = 0.01;

  auto const calc_delta = [&ft](auto const axis) {
    return axis * ft.delta_millis() * CONTROLLER_SENSITIVITY;
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

  if (c.button_a()) {
    std::cerr << "BUTTON A\n";
    try_pickup_nearby_item(state, ft);
  }
  if (c.button_b()) {
    std::cerr << "BUTTON B\n";
  }
  if (c.button_x()) {
    std::cerr << "BUTTON X\n";
  }
  if (c.button_y()) {
    std::cerr << "BUTTON Y\n";
  }

  if (c.button_back()) {
    std::cerr << "BUTTON BACK\n";
  }
  if (c.button_guide()) {
    std::cerr << "BUTTON GUIDE\n";
  }
  if (c.button_start()) {
    std::cerr << "BUTTON START\n";
  }

  if (c.button_left_joystick()) {
    std::cerr << "BUTTON LEFT JOYSTICK\n";
  }
  if (c.button_right_joystick()) {
    std::cerr << "BUTTON RIGHT JOYSTICK\n";
  }

  if (c.button_left_shoulder()) {
    std::cerr << "BUTTON LEFT SHOULDER\n";
  }
  if (c.button_right_shoulder()) {
    std::cerr << "BUTTON RIGHT SHOULDER\n";
  }

  if (c.button_dpad_down()) {
    std::cerr << "BUTTON DPAD DOWN\n";
  }
  if (c.button_dpad_up()) {
    std::cerr << "BUTTON DPAD UP\n";
  }
  if (c.button_dpad_left()) {
    std::cerr << "BUTTON DPAD LEFT\n";
  }
  if (c.button_dpad_right()) {
    std::cerr << "BUTTON DPAD RIGHT\n";
  }
}

bool
process_event(GameState &state, SDL_Event &event, FrameTime const& ft)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;

  // If the user pressed enter, don't process mouse events (for the game)
  auto &ui = es.ui_state.debug;

  auto const type = event.type;
  bool const enter_pressed = event.key.keysym.sym == SDLK_RETURN;
  if((type == SDL_KEYDOWN) && enter_pressed) {
    ui.enter_pressed ^= true;
  }

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
