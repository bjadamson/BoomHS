#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/io.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/mouse_picker.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

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
try_pickup_nearby_item(GameState& state, FrameTime const& ft)
{
  auto& lm       = state.level_manager;
  auto& es       = state.engine_state;
  auto& logger   = es.logger;
  auto& active   = lm.active();
  auto& registry = active.registry;

  auto const  player_eid       = find_player(registry);
  auto&       player           = registry.get<Player>(player_eid);

  auto&       player_transform = player.world_object.transform();
  auto const& player_pos       = player_transform.translation;
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

    player.pickup_entity(eid, registry);

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
process_mousemotion(GameState& state, Player& player, SDL_MouseMotionEvent const& motion,
                    Camera& camera, FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ms     = es.mouse_states.current;
  auto& ui     = es.ui_state.debug;

  {
    ms.relative.x = motion.xrel;
    ms.relative.y = motion.yrel;

    ms.coords.x = motion.x;
    ms.coords.y = motion.y;
  }

  auto& wo = player.world_object;
  if (camera.mode() == CameraMode::FPS) {
    auto const& sens = ms.sensitivity;
    float const dx   = sens.x * ms.relative.x;
    float const dy   = sens.y * ms.relative.y;
    camera.rotate(dx, 0.0f);
    auto& movement = player.movement;
    wo.rotate_to_match_camera_rotation(camera);
  }
  else if (camera.mode() == CameraMode::ThirdPerson) {
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
      wo.rotate_degrees(x_dt, opengl::Y_UNIT_VECTOR);
    }
  }
}

enum class MouseButton
{
  LEFT,
  RIGHT
};

void
select_mouse_under_cursor(FrameState& fstate, MouseButton const mb)
{
  auto& es      = fstate.es;
  auto& logger  = es.logger;
  auto& uistate = es.ui_state.debug;

  auto& zs       = fstate.zs;
  auto& registry = zs.registry;

  MousePicker      mouse_picker;
  glm::vec3 const  ray_dir   = mouse_picker.calculate_ray(fstate);
  glm::vec3 const& ray_start = fstate.camera_world_position();

  auto const eids = find_all_entities_with_component<Selectable>(registry);
  for (auto const eid : eids) {
    auto& selectable = registry.get<Selectable>(eid);
    selectable.selected = false;
  }

  std::vector<std::pair<EntityID, float>> distances;
  for (auto const eid : eids) {
    auto bbox_copy       = registry.get<AABoundingBox>(eid);
    auto const& transform  = registry.get<Transform>(eid);
    auto&       selectable = registry.get<Selectable>(eid);

    //Ray const  ray{ray_start, ray_dir};
    //bool const intersects = collision::ray_box_intersect(ray, transform, bbox);

    auto const model_matrix = transform.model_matrix();

    float distance = 0.0f;
    bbox_copy.min *= transform.scale;
    bbox_copy.max *= transform.scale;
    bool const intersects = collision::ray_obb_intersection(ray_start, ray_dir, bbox_copy.min, bbox_copy.max,
        model_matrix, distance);

    if (intersects) {
      distances.emplace_back(PAIR(eid, distance));
    }
  }
  bool const something_selected = !distances.empty();
  if (something_selected) {
    auto const cmp = [](auto const& l, auto const& r) { return l.second < r.second; };
    std::sort(distances.begin(), distances.end(), cmp);
    auto const& pair = mb == MouseButton::LEFT
      ? distances.front()
      : distances.back();

    auto const eid = pair.first;
    registry.get<Selectable>(eid).selected = true;

    auto const name = registry.has<Name>(eid)
      ? registry.get<Name>(eid).value
      : "Unnamed";
    LOG_ERROR_SPRINTF("(FOUND INTERSECTION) eid: %lu name: %s, distance %f", eid, name, pair.second);
  }
  else {
    LOG_ERROR("NO INTERSECTION");
  }
}

void
process_mousebutton_down(GameState& state, Player& player, SDL_MouseButtonEvent const& event, Camera& camera,
                         FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ms     = es.mouse_states.current;

  auto& zs = state.level_manager.active();

  auto const& button = event.button;
  ms.left_pressed = button == SDL_BUTTON_LEFT;
  ms.right_pressed = button == SDL_BUTTON_RIGHT;

  if (ms.either_pressed()) {
    auto const cstate = CameraFrameState::from_camera(camera);
    FrameState fstate{cstate, es, zs};
    if (ms.left_pressed) {
      select_mouse_under_cursor(fstate, MouseButton::LEFT);
    }
    else if (ms.right_pressed) {
      select_mouse_under_cursor(fstate, MouseButton::RIGHT);
    }
  }
  else if (button == SDL_BUTTON_RIGHT) {
    ms.right_pressed = true;
  }
  if (button == SDL_BUTTON_MIDDLE) {
    LOG_INFO("toggling mouse up/down (pitch) lock");
    camera.rotate_lock ^= true;
  }
}

void
process_mousebutton_up(GameState& state, Player& player, SDL_MouseButtonEvent const& event, Camera& camera,
                       FrameTime const& ft)
{
  auto& es = state.engine_state;
  auto& ms = es.mouse_states.current;

  auto const& button = event.button;
  if (SDL_BUTTON_LEFT == button) {
    ms.left_pressed = false;
  }
  else if (SDL_BUTTON_RIGHT == button) {
    ms.right_pressed = false;
  }
}

void
process_keyup(GameState& state, Player& player, SDL_Event const& event, Camera& camera, FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;

  auto& wo = player.world_object;
  auto& movement = player.movement;

  switch (event.key.keysym.sym) {
    break;
  }
}

void
process_keydown(GameState& state, Player& player, SDL_Event const& event, Camera& camera, FrameTime const& ft)
{
  auto& es      = state.engine_state;
  auto& logger  = es.logger;
  auto& uistate = es.ui_state;

  auto& lm             = state.level_manager;
  auto& active         = lm.active();
  auto& registry       = active.registry;
  auto& ldata          = active.level_data;

  auto& nbt = ldata.nearby_targets;

  auto& debug = uistate.debug;
  auto& ingame = uistate.ingame;
  auto& chat_state = ingame.chat_state;
  auto& player_wo = player.world_object;
  auto& movement = player.movement;

  auto const rotate_player = [&](float const angle, glm::vec3 const& axis) {
    player_wo.rotate_degrees(angle, axis);
  };
  switch (event.key.keysym.sym) {
  case SDLK_RETURN:
    // Toggle whether or not the user is editing, but force the yscroll position to reset.
    chat_state.currently_editing ^= true;
    chat_state.reset_yscroll_position = true;
    break;
  case SDLK_e:
    if (event.key.keysym.mod & KMOD_CTRL) {
      debug.show_entitywindow ^= true;
    }
    else {
      try_pickup_nearby_item(state, ft);
    }
    // move_up(state, player_wo, ft);
    break;
  case SDLK_q:
    // move_down(state, player_wo, ft);
    break;
  case SDLK_F11:
    uistate.draw_debug_ui ^= true;
    break;
  case SDLK_t:
    // invert
    camera.next_mode();
    break;
  case SDLK_TAB: {
    uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
    CycleDirection const dir = keystate[SDL_SCANCODE_LSHIFT]
      ? CycleDirection::Forward
      : CycleDirection::Backward;
    nbt.cycle(dir, ft);
  } break;
  case SDLK_BACKQUOTE: {
    auto&      inventory = player.inventory;
    inventory.toggle_open();
  } break;
  case SDLK_SPACE: {
    auto const  player_eid = find_player(registry);
    auto& player           = registry.get<Player>(player_eid);
    auto const selected_opt = nbt.selected();

    // Toggle the state trackerwhether or not the player is attacking
    // AND
    // If the player has an entity selected, try and attack it.
    LOG_ERROR("SPACE BAR PRESSED");
    if (selected_opt) {
      EntityID const target_eid = *selected_opt;
      auto& target = registry.get<NPCData>(target_eid);
      if (NPC::is_dead(target.health)) {
        LOG_ERROR("TARGET IS DEAD");
        break;
      }
      else {
        player.is_attacking ^= true;
      }
    }
    else {
      assert(!player.is_attacking);
    }
  } break;
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
process_mousewheel(GameState& state, Player& player, SDL_MouseWheelEvent const& wheel, Camera& camera,
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
  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& mss    = es.mouse_states;

  auto& ms_now  = mss.current;
  auto& ms_prev = mss.previous;

  bool const both_yes_now = ms_now.both_pressed();
  bool const both_yes_prev = ms_prev.both_pressed();
  bool const both_not_prev = !both_yes_prev;

  auto& lm     = state.level_manager;
  auto& registry  = lm.active().registry;

  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);
  auto& wo = player.world_object;

  auto& movement = player.movement;
  if (both_yes_now) {
    wo.rotate_to_match_camera_rotation(camera);
    movement.mouse_forward = wo.world_forward();
  }
  else {
    movement.mouse_forward = glm::vec3{0};
  }
}

void
process_keystate(GameState& state, Camera& camera, FrameTime const& ft)
{
  // continual keypress responses procesed here
  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);

  auto& es       = state.engine_state;
  auto& logger = es.logger;
  auto& lm       = state.level_manager;
  auto& zs       = lm.active();
  auto& ldata    = zs.level_data;
  auto& registry = zs.registry;

  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);
  auto &movement = player.movement;

  auto constexpr ZERO = glm::vec3{0};
  auto& wo = player.world_object;

  movement.forward = keystate[SDL_SCANCODE_W]
    ? wo.world_forward()
    : ZERO;
  movement.backward = keystate[SDL_SCANCODE_S]
    ? wo.world_backward()
    : ZERO;

  movement.left = keystate[SDL_SCANCODE_A]
    ? wo.world_left()
    : ZERO;

  movement.right = keystate[SDL_SCANCODE_D]
    ? wo.world_right()
    : ZERO;
}

void
process_controllerstate(GameState& state, SDLControllers const& controllers, Camera& camera,
                        FrameTime const& ft)
{
  if (controllers.empty()) {
    return;
  }

  auto& es     = state.engine_state;
  auto& lm     = state.level_manager;
  auto& registry  = lm.active().registry;

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

  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);
  auto& player_wo = player.world_object;

  auto constexpr THRESHOLD  = 0.4f;
  auto const less_threshold = [&](auto const& v) { return v <= 0 && (v <= AXIS_MIN * THRESHOLD); };
  auto const greater_threshold = [](auto const& v) {
    return v >= 0 && (v >= AXIS_MAX * THRESHOLD);
  };

  auto& movement = player.movement;
  auto const axis_left_x = c.axis_left_x();
  if (less_threshold(axis_left_x)) {
    movement.left = player_wo.world_left();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.left = glm::vec3{0};
  }
  if (greater_threshold(axis_left_x)) {
    movement.right = player_wo.world_right();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.right = glm::vec3{0};
  }
  auto const axis_left_y = c.axis_left_y();
  if (less_threshold(axis_left_y)) {
    movement.forward = player_wo.world_forward();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.forward = glm::vec3{0};
  }
  if (greater_threshold(axis_left_y)) {
    movement.backward = player_wo.world_backward();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.backward = glm::vec3{0};
  }

  auto constexpr CONTROLLER_SENSITIVITY = 0.01;

  auto const calc_delta = [&ft](auto const axis) {
    return axis * ft.delta_millis() * CONTROLLER_SENSITIVITY;
  };
  {
    auto const axis_right_x = c.axis_right_x();
    if (less_threshold(axis_right_x)) {
      LOG_ERROR("LT");
      float const dx = calc_delta(axis_right_x);
      camera.rotate(dx, 0.0);
      player_wo.rotate_to_match_camera_rotation(camera);
    }
    if (greater_threshold(axis_right_x)) {
      LOG_ERROR("GT");
      float const dx = calc_delta(axis_right_x);
      camera.rotate(dx, 0.0);
      player_wo.rotate_to_match_camera_rotation(camera);
    }
  }
  {
    auto const right_axis_y = c.axis_right_y();
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
    LOG_DEBUG("BUTTON A\n");
    try_pickup_nearby_item(state, ft);
  }
  if (c.button_b()) {
    LOG_DEBUG("BUTTON B\n");
  }
  if (c.button_x()) {
    LOG_DEBUG("BUTTON X\n");
  }
  if (c.button_y()) {
    LOG_DEBUG("BUTTON Y\n");
  }

  if (c.button_back()) {
    LOG_DEBUG("BUTTON BACK\n");
  }
  if (c.button_guide()) {
    LOG_DEBUG("BUTTON GUIDE\n");
  }
  if (c.button_start()) {
    LOG_DEBUG("BUTTON START\n");
  }

  // joystick buttons
  if (c.button_left_joystick()) {
    LOG_DEBUG("BUTTON LEFT JOYSTICK\n");
  }
  if (c.button_right_joystick()) {
    LOG_DEBUG("BUTTON RIGHT JOYSTICK\n");
  }

  // shoulder buttons
  if (c.button_left_shoulder()) {
    LOG_DEBUG("BUTTON LEFT SHOULDER\n");
  }
  if (c.button_right_shoulder()) {
    LOG_DEBUG("BUTTON RIGHT SHOULDER\n");
  }

  // trigger buttons
  if (c.button_left_trigger()) {
    LOG_DEBUG("BUTTON LEFT TRIGGER\n");
  }
  if (c.button_right_trigger()) {
    LOG_DEBUG("BUTTON RIGHT TRIGGER\n");
  }

  // d-pad buttons
  if (c.button_dpad_down()) {
    LOG_DEBUG("BUTTON DPAD DOWN\n");
  }
  if (c.button_dpad_up()) {
    LOG_DEBUG("BUTTON DPAD UP\n");
  }

  auto& zs       = lm.active();
  auto& ldata    = zs.level_data;
  auto& nbt = ldata.nearby_targets;
  if (c.button_dpad_left()) {
    LOG_DEBUG("BUTTON DPAD LEFT\n");
    nbt.cycle_backward(ft);
  }
  if (c.button_dpad_right()) {
    LOG_DEBUG("BUTTON DPAD RIGHT\n");
    nbt.cycle_forward(ft);
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

  auto& lm     = state.level_manager;
  auto& active = lm.active();
  auto& registry = active.registry;

  auto const player_eid = find_player(registry);
  auto& player = registry.get<Player>(player_eid);

  auto& ui     = es.ui_state;
  auto& ingame = ui.ingame;
  auto& chat_buffer = ingame.chat_buffer;
  auto& currently_editing = ingame.chat_state.currently_editing;

  auto const type               = event.type;
  bool const event_type_keydown = type == SDL_KEYDOWN;
  auto const key_pressed        = event.key.keysym.sym;
  if (event_type_keydown) {
    switch (key_pressed) {
      case SDLK_F10:
        es.quit = true;
        return;
      case SDLK_ESCAPE:
        {
          auto& ldata  = active.level_data;
          auto& nbt = ldata.nearby_targets;
          if (currently_editing) {
            chat_buffer.clear();
            currently_editing = false;
          }
          else if (player.is_attacking) {
            player.is_attacking = false;
          }
          else if (nbt.selected()) {
            nbt.clear();
          }
          else {
            es.main_menu.show ^= true;
          }
          return;
        }
        break;
    }
  }

  // If the user pressed enter, don't process mouse events (for the game)
  if (currently_editing) {
    return;
  }

  auto& imgui = es.imgui;
  if (imgui.WantCaptureMouse || imgui.WantCaptureKeyboard) {
    return;
  }

  switch (type) {
  case SDL_MOUSEBUTTONDOWN:
    process_mousebutton_down(state, player, event.button, camera, ft);
    break;
  case SDL_MOUSEBUTTONUP:
    process_mousebutton_up(state, player, event.button, camera, ft);
    break;
  case SDL_MOUSEMOTION:
    process_mousemotion(state, player, event.motion, camera, ft);
    break;
  case SDL_MOUSEWHEEL:
    process_mousewheel(state, player, event.wheel, camera, ft);
    break;
  case SDL_KEYDOWN:
    process_keydown(state, player, event, camera, ft);
    break;
  case SDL_KEYUP:
    process_keyup(state, player, event, camera, ft);
    break;
  }
}

void
IO::process(GameState& state, SDLControllers const& controllers, Camera& camera,
            FrameTime const& ft)
{
  auto& es         = state.engine_state;
  auto& uistate    = es.ui_state;
  auto& ingame     = uistate.ingame;
  auto& chat_state = ingame.chat_state;

  if (chat_state.currently_editing) {
    return;
  }

  process_mousestate(state, camera, ft);
  process_keystate(state, camera, ft);
  if (!state.engine_state.disable_controller_input) {
    // TODO: using controller and keyboard input at the same time does not work.
    // reason: The controller when it's stick's aren't activated, every frame, set's the same
    // variables to the keyboard controller would use to 0, effectively nullifying any input the
    // keyboard can do.
    //
    // Idea: We could use separate vector's for tracking the controller input, if we want to allow
    // both at the same time (why?).
    process_controllerstate(state, controllers, camera, ft);
  }
}

} // namespace boomhs
