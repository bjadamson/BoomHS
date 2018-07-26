#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mouse_picker.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <opengl/frame.hpp>

#include <window/controller.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/log.hpp>
#include <stlw/math.hpp>

#include <extlibs/imgui.hpp>
#include <iostream>

float constexpr MOVE_DISTANCE = 1.0f;
float constexpr SCALE_FACTOR  = 0.20f;
float constexpr ZOOM_FACTOR   = 0.2f;

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

void
move_worldobject(GameState& state, glm::vec3 (WorldObject::*fn)() const, WorldObject& wo,
                 FrameTime const& ft)
{
  auto& es = state.engine_state;

  auto&            lm           = state.level_manager;
  auto&            zs           = lm.active();
  LevelData const& ldata        = zs.level_data;
  auto const&      terrain_grid = ldata.terrain;

  auto const max_pos = terrain_grid.max_worldpositions();
  auto const max_x   = max_pos.x;
  auto const max_z   = max_pos.y;

  auto&           logger   = es.logger;
  glm::vec3 const move_vec = (wo.*fn)();

  glm::vec3 const delta  = move_vec * wo.speed() * ft.delta_millis();
  glm::vec3 const newpos = wo.world_position() + delta;

  bool const x_outofbounds = newpos.x >= max_x || newpos.x < 0;
  bool const y_outofbounds = newpos.z >= max_z || newpos.z < 0;
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
    auto const new_x = flip_sides(newpos.x, 0ul, max_x);
    wo.move_to(new_x, 0.0, newpos.z);
  }
  else if (y_outofbounds) {
    auto const new_z = flip_sides(newpos.z, 0ul, max_z);
    wo.move_to(newpos.x, 0.0, new_z);
  }
  else {
    wo.move(delta);
  }
}

void
move_forward(GameState& state, FrameTime const& ft)
{
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;

  move_worldobject(state, &WorldObject::world_forward, player, ft);
}

void
move_backward(GameState& state, FrameTime const& ft)
{
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;
  move_worldobject(state, &WorldObject::world_backward, player, ft);
}

void
move_left(GameState& state, FrameTime const& ft)
{
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;
  move_worldobject(state, &WorldObject::world_left, player, ft);
}

void
move_right(GameState& state, FrameTime const& ft)
{
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;
  move_worldobject(state, &WorldObject::world_right, player, ft);
}

void
move_up(GameState& state, FrameTime const& ft)
{
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;
  move_worldobject(state, &WorldObject::world_up, player, ft);
}

void
move_down(GameState& state, FrameTime const& ft)
{
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;
  move_worldobject(state, &WorldObject::world_down, player, ft);
}

void
try_pickup_nearby_item(GameState& state, FrameTime const& ft)
{
  auto& lm       = state.level_manager;
  auto& es       = state.engine_state;
  auto& logger   = es.logger;
  auto& active   = lm.active();
  auto& registry = active.registry;

  auto const  player_eid       = find_player(registry);
  auto&       player_transform = registry.get<Transform>(player_eid);
  auto const& player_pos       = player_transform.translation;
  auto&       player           = registry.get<PlayerData>(player_eid);
  auto&       inventory        = player.inventory;

  static constexpr auto MINIMUM_DISTANCE_TO_PICKUP = 1.0f;
  auto const            items                      = find_items(registry);
  for (EntityID const eid : items) {
    Item& item = registry.get<Item>(eid);
    if (item.is_pickedup) {
      LOG_INFO("item already picked up.\n");
      continue;
    }

    auto&       item_transform = registry.get<Transform>(eid);
    auto const& item_pos       = item_transform.translation;
    auto const  distance       = glm::distance(item_pos, player_pos);

    if (distance > MINIMUM_DISTANCE_TO_PICKUP) {
      LOG_INFO("There is nothing nearby to pickup.");
      continue;
    }

    Player::pickup_entity(eid, registry);

    if (registry.has<Torch>(eid)) {
      auto& pointlight = registry.get<PointLight>(eid);
      pointlight.attenuation /= 3.0f;

      LOG_INFO("You have picked up a torch.");
    }
    else {
      LOG_INFO("You have picked up an item.");
    }
  }
}

void
process_mousemotion(GameState& state, SDL_MouseMotionEvent const& motion, Camera& camera,
                    FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ms     = es.mouse_state;
  auto& ui     = es.ui_state.debug;

  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;

  {
    // update the mouse relative and current positions
    ms.relative.x = motion.xrel;
    ms.relative.y = motion.yrel;

    ms.coords.x = motion.x;
    ms.coords.y = motion.y;
  }

  if (ms.both_pressed()) {
    player.rotate_to_match_camera_rotation(camera);
    move_forward(state, ft);
  }
  if (ms.left_pressed) {
    auto const& sens = ms.sensitivity;
    float const dx   = sens.x * ms.relative.x;
    float const dy   = sens.y * ms.relative.y;
    camera.rotate(dx, dy);
  }
  if (ms.right_pressed) {
    float const speed = camera.rotation_speed;
    float const angle = ms.relative.x > 0 ? speed : -speed;

    auto const x_dt     = angle * ft.delta_millis();
    auto constexpr y_dt = 0.0f;
    player.rotate_degrees(x_dt, opengl::Y_UNIT_VECTOR);
  }
}

void
select_mouse_under_cursor(FrameState& fstate)
{
  auto& es      = fstate.es;
  auto& logger  = es.logger;
  auto& uistate = es.ui_state.debug;

  auto& zs       = fstate.zs;
  auto& registry = zs.registry;

  MousePicker      mouse_picker;
  glm::vec3 const  ray_dir   = mouse_picker.calculate_ray(fstate);
  glm::vec3 const& ray_start = fstate.camera_world_position();

  auto const components_bbox =
      find_all_entities_with_component<AABoundingBox, Transform, Selectable>(registry);
  for (auto const eid : components_bbox) {
    auto const& bbox       = registry.get<AABoundingBox>(eid);
    auto const& transform  = registry.get<Transform>(eid);
    auto&       selectable = registry.get<Selectable>(eid);

    Ray const  ray{ray_start, ray_dir};
    bool const intersects = collision::ray_box_intersect(ray, transform, bbox);
    selectable.selected   = intersects;

    // auto&            tree_transform = registry.get<Transform>(eid);
    // glm::vec3 const& sphere_center  = tree_transform.translation;
    // float const      rad_squared    = stlw::math::squared(bbox.radius);

    // float      distance = 0.0f;
    // bool const intersects =
    // glm::intersectRaySphere(ray_start, ray_dir, sphere_center, rad_squared, distance);
    if (intersects) {
      LOG_ERROR_SPRINTF("intersects (YES) %i", intersects);
      uistate.selected_entity = static_cast<int>(eid);
    }
  }
}

void
process_mousebutton_down(GameState& state, SDL_MouseButtonEvent const& event, Camera& camera,
                         FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ms     = es.mouse_state;

  auto& zs = state.level_manager.active();

  auto const& button = event.button;
  if (button == SDL_BUTTON_LEFT) {
    ms.left_pressed = true;

    auto const fmatrices = FrameMatrices::from_camera(camera);
    FrameState fstate{fmatrices, es, zs};
    select_mouse_under_cursor(fstate);
  }
  else if (button == SDL_BUTTON_RIGHT) {
    ms.right_pressed = true;
  }
  if (button == SDL_BUTTON_MIDDLE) {
    LOG_INFO("toggling mouse up/down (pitch) lock");
    camera.rotate_lock ^= true;

    auto& lm     = state.level_manager;
    auto& ldata  = lm.active().level_data;
    auto& player = ldata.player;

    player.rotate_to_match_camera_rotation(camera);
  }
}

void
process_mousebutton_up(GameState& state, SDL_MouseButtonEvent const& event, Camera& camera,
                       FrameTime const& ft)
{
  auto& es = state.engine_state;
  auto& ms = es.mouse_state;

  auto const& button = event.button;
  if (SDL_BUTTON_LEFT == button) {
    ms.left_pressed = false;
  }
  else if (SDL_BUTTON_RIGHT == button) {
    ms.right_pressed = false;
  }
}

void
process_keyup(GameState& state, SDL_Event const& event, Camera& camera, FrameTime const& ft)
{
}

void
process_keydown(GameState& state, SDL_Event const& event, Camera& camera, FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ui     = es.ui_state;

  auto& lm             = state.level_manager;
  auto& active         = lm.active();
  auto& ldata          = active.level_data;
  auto& player         = ldata.player;
  auto& nearby_targets = ldata.nearby_targets;

  auto const rotate_player = [&](float const angle, glm::vec3 const& axis) {
    player.rotate_degrees(angle, axis);
  };
  switch (event.key.keysym.sym) {
  case SDLK_ESCAPE:
    es.main_menu.show ^= true;
    break;
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
    if (event.key.keysym.mod & KMOD_CTRL) {
      auto& uistate = es.ui_state.debug;
      uistate.show_entitywindow ^= true;
    }
    else {
      try_pickup_nearby_item(state, ft);
    }
    // move_up(state, ft);
    break;
  case SDLK_q:
    // move_down(state, ft);
    break;
  case SDLK_F10:
    es.quit = true;
    break;
  case SDLK_F11:
    ui.draw_debug_ui ^= true;
    break;
  case SDLK_t:
    // invert
    camera.next_mode();
    break;
  case SDLK_TAB: {
    uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
    assert(keystate);

    if (!keystate[SDL_SCANCODE_LSHIFT]) {
      nearby_targets.cycle_forward(ft);
    }
    else {
      nearby_targets.cycle_backward(ft);
    }
  } break;
  case SDLK_BACKQUOTE: {
    auto&      registry  = active.registry;
    auto const eid       = find_player(registry);
    auto&      player    = registry.get<PlayerData>(eid);
    auto&      inventory = player.inventory;
    inventory.toggle_open();
  } break;
  case SDLK_SPACE:
    break;
  // scaling
  case SDLK_KP_PLUS:
  case SDLK_o:
    // et.scale_entities(sf(SCALE_FACTOR));
    break;
  case SDLK_KP_MINUS:
    // z-rotation
    break;
  case SDLK_j: {
    auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};
    // et.rotate_entities(ANGLE, ROTATION_VECTOR);
    break;
  }
  case SDLK_k: {
    auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};
    // et.rotate_entities(-ANGLE, ROTATION_VECTOR);
    break;
  }
  // y-rotation
  case SDLK_u: {
    auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
    // et.rotate_entities(ANGLE, ROTATION_VECTOR);
    break;
  }
  case SDLK_i: {
    auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
    // et.rotate_entities(-ANGLE, ROTATION_VECTOR);
    break;
  }
  // x-rotation
  case SDLK_n: {
    auto constexpr ROTATION_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
    // et.rotate_entities(ANGLE, ROTATION_VECTOR);
    break;
  }
  case SDLK_m: {
    auto constexpr ROTATION_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
    // et.rotate_entities(-ANGLE, ROTATION_VECTOR);
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
process_mousewheel(GameState& state, SDL_MouseWheelEvent const& wheel, Camera& camera,
                   FrameTime const& ft)
{
  auto& logger = state.engine_state.logger;
  LOG_TRACE("mouse wheel event detected.");

  auto& lm    = state.level_manager;
  auto& ldata = lm.active().level_data;
  if (wheel.y > 0) {
    camera.decrease_zoom(ZOOM_FACTOR);
  }
  else {
    camera.increase_zoom(ZOOM_FACTOR);
  }
}

void
process_mousestate(GameState& state, Camera& camera, FrameTime const& ft)
{
  auto& es = state.engine_state;
  auto& ms = es.mouse_state;
  if (ms.both_pressed()) {

    auto& lm     = state.level_manager;
    auto& ldata  = lm.active().level_data;
    auto& player = ldata.player;

    move_worldobject(state, &WorldObject::world_forward, player, ft);
  }
}

void
process_keystate(GameState& state, Camera& camera, FrameTime const& ft)
{
  // continual keypress responses procesed here
  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);

  auto& es     = state.engine_state;
  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;

  if (keystate[SDL_SCANCODE_W]) {
    // move_forward(state, ft);
  }
  if (keystate[SDL_SCANCODE_S]) {
    // move_backward(state, ft);
  }
  if (keystate[SDL_SCANCODE_A]) {
    // move_left(state, ft);
  }
  if (keystate[SDL_SCANCODE_D]) {
    // move_right(state, ft);
  }
  if (keystate[SDL_SCANCODE_Q]) {
    // move_up(state, ft);
  }
  if (keystate[SDL_SCANCODE_E]) {
    // move_down(state, ft);
  }
}

void
process_controllefstate(GameState& state, SDLControllers const& controllers, Camera& camera,
                        FrameTime const& ft)
{
  if (controllers.empty()) {
    return;
  }
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& c      = controllers.first();

  SDL_Joystick* joystick = c.joystick;
  assert(joystick);

  auto const read_axis = [&c](auto const axis) {
    return SDL_GameControllerGetAxis(c.controller.get(), axis);
  };

  // https://wiki.libsdl.org/SDL_GameControllerGetAxis
  //
  // using 32bit ints to be sure no overflow (maybe unnecessary?)
  int32_t constexpr AXIS_MIN = -32768;
  int32_t constexpr AXIS_MAX = 32767;

  auto& lm     = state.level_manager;
  auto& ldata  = lm.active().level_data;
  auto& player = ldata.player;

  auto constexpr THRESHOLD  = 0.4f;
  auto const less_threshold = [](auto const& v) { return v <= 0 && (v <= AXIS_MIN * THRESHOLD); };
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
    LOG_INFO("BUTTON A\n");
    try_pickup_nearby_item(state, ft);
  }
  if (c.button_b()) {
    LOG_INFO("BUTTON B\n");
  }
  if (c.button_x()) {
    LOG_INFO("BUTTON X\n");
  }
  if (c.button_y()) {
    LOG_INFO("BUTTON Y\n");
  }

  if (c.button_back()) {
    LOG_INFO("BUTTON BACK\n");
  }
  if (c.button_guide()) {
    LOG_INFO("BUTTON GUIDE\n");
  }
  if (c.button_start()) {
    LOG_INFO("BUTTON START\n");
  }

  if (c.button_left_joystick()) {
    LOG_INFO("BUTTON LEFT JOYSTICK\n");
  }
  if (c.button_right_joystick()) {
    LOG_INFO("BUTTON RIGHT JOYSTICK\n");
  }

  if (c.button_left_shoulder()) {
    LOG_INFO("BUTTON LEFT SHOULDER\n");
  }
  if (c.button_right_shoulder()) {
    LOG_INFO("BUTTON RIGHT SHOULDER\n");
  }

  if (c.button_dpad_down()) {
    LOG_INFO("BUTTON DPAD DOWN\n");
  }
  if (c.button_dpad_up()) {
    LOG_INFO("BUTTON DPAD UP\n");
  }
  if (c.button_dpad_left()) {
    LOG_INFO("BUTTON DPAD LEFT\n");
  }
  if (c.button_dpad_right()) {
    LOG_INFO("BUTTON DPAD RIGHT\n");
  }
}

} // namespace

namespace boomhs
{

void
IO::process_event(GameState& state, SDL_Event& event, Camera& camera, FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;

  // If the user pressed enter, don't process mouse events (for the game)
  auto& ui = es.ui_state.debug;

  auto& imgui = es.imgui;
  if (imgui.WantCaptureMouse || imgui.WantCaptureKeyboard) {
    return;
  }

  auto const type          = event.type;
  bool const enter_pressed = event.key.keysym.sym == SDLK_RETURN;
  if ((type == SDL_KEYDOWN) && enter_pressed) {
    ui.enter_pressed ^= true;
  }

  if (ui.block_input || ui.enter_pressed) {
    return;
  }

  switch (event.type) {
  case SDL_MOUSEBUTTONDOWN:
    process_mousebutton_down(state, event.button, camera, ft);
    break;
  case SDL_MOUSEBUTTONUP:
    process_mousebutton_up(state, event.button, camera, ft);
    break;
  case SDL_MOUSEMOTION:
    process_mousemotion(state, event.motion, camera, ft);
    break;
  case SDL_MOUSEWHEEL:
    process_mousewheel(state, event.wheel, camera, ft);
    break;
  case SDL_KEYDOWN:
    process_keydown(state, event, camera, ft);
    break;
  case SDL_KEYUP:
    process_keyup(state, event, camera, ft);
    break;
  }
}

void
IO::process(GameState& state, SDLControllers const& controllers, Camera& camera,
            FrameTime const& ft)
{
  process_mousestate(state, camera, ft);
  process_keystate(state, camera, ft);
  process_controllefstate(state, controllers, camera, ft);
}

} // namespace boomhs
