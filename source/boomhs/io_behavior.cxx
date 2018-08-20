#include <boomhs/io_behavior.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/math.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/player.hpp>
#include <boomhs/raycast.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

float constexpr ZOOM_FACTOR = 0.2f;

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;
using namespace opengl;

namespace
{

void
fps_mousemove(WorldObject& wo, Camera& camera, WorldObject& player,
              DeviceSensitivity const& dsens, float const xrel, float const yrel,
              FrameTime const& ft)
{
  camera.rotate(xrel, yrel, dsens, ft);
}

void
thirdperson_mousemove(WorldObject& wo, Camera& camera, MouseState const& ms,
                      DeviceSensitivity const& dsens, float const xrel, float const yrel,
                      FrameTime const& ft)
{
  if (ms.left_pressed()) {
    camera.rotate(xrel, yrel, dsens, ft);
  }
  if (ms.right_pressed()) {
    auto constexpr  RIGHTCLICK_TURN_SPEED_DEGREES = 60.0f;
    float constexpr speed = RIGHTCLICK_TURN_SPEED_DEGREES;
    float const angle = xrel > 0 ? speed : -speed;

    auto const x_dt  = angle * ft.delta_millis();
    wo.rotate_degrees(glm::degrees(x_dt), EulerAxis::Y);
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

  glm::vec3 const  ray_dir   = Raycast::calculate_ray(fstate);
  glm::vec3 const& ray_start = fstate.camera_world_position();

  std::vector<std::pair<EntityID, float>> distances;
  auto const eids = find_all_entities_with_component<Selectable>(registry);
  for (auto const eid : eids) {
    auto const& bbox = registry.get<AABoundingBox>(eid).cube;
    auto const& tr   = registry.get<Transform>(eid);
    auto&       sel  = registry.get<Selectable>(eid);
    sel.selected = false;

    bool const can_use_simple_test = (tr.rotation == glm::quat{}) && (tr.scale == ONE);

    float distance = 0.0f;
    bool intersects = false;
    if (can_use_simple_test) {
      Ray const  ray{ray_start, ray_dir};
      intersects = collision::ray_cube_intersect(ray, tr, bbox, distance);
    }
    else {
      intersects = collision::ray_obb_intersection(ray_start, ray_dir, bbox, tr, distance);
    }

    if (intersects) {
      distances.emplace_back(PAIR(eid, distance));
      LOG_TRACE_SPRINTF("Intersection found using %s test, distance %f", can_use_simple_test ? "SIMPLE" : "COMPLEX", distance);
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
  }
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// PlayerPlayingGameBehavior
void
PlayerPlayingGameBehavior::mousebutton_down(MouseButtonEvent&& mbe)
{
  auto& state       = mbe.game_state;
  auto& camera      = mbe.camera;
  auto const& event = mbe.event;
  auto const& ft    = mbe.frame_time;
  auto& player      = mbe.player;

  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ms     = es.device_states.mouse.current;

  auto& zs = state.level_manager.active();
  auto& uistate = es.ui_state.debug;

  auto const& button = event.button;

  if (ms.either_pressed()) {
    auto const cstate = CameraFrameState::from_camera(camera);
    FrameState fstate{cstate, es, zs};
    if (!uistate.lock_debugselected) {
      if (ms.left_pressed()) {
        select_mouse_under_cursor(fstate, MouseButton::LEFT);
      }
      else if (ms.right_pressed()) {
        select_mouse_under_cursor(fstate, MouseButton::RIGHT);
      }
    }
  }
  if (button == SDL_BUTTON_MIDDLE) {
    LOG_INFO("toggling mouse up/down (pitch) lock");
    camera.toggle_rotation_lock();
  }
}

void
PlayerPlayingGameBehavior::mousebutton_up(MouseButtonEvent&& mbe)
{
  auto& state       = mbe.game_state;
  auto& camera      = mbe.camera;
  auto const& event = mbe.event;
  auto const& ft    = mbe.frame_time;
  auto& player      = mbe.player;

  auto& es = state.engine_state;
  auto& ms = es.device_states.mouse.current;
}

void
PlayerPlayingGameBehavior::mouse_wheel(MouseWheelEvent &&mwe)
{
  auto& state    = mwe.game_state;
  auto& camera   = mwe.camera;
  auto& wheel    = mwe.wheel;
  auto& player   = mwe.player;
  auto const& ft = mwe.frame_time;

  auto& logger = state.engine_state.logger;
  LOG_TRACE("mouse wheel event detected.");

  auto& lm    = state.level_manager;
  auto& ldata = lm.active().level_data;

  auto& arcball = camera.arcball;
  if (wheel.y > 0) {
    arcball.decrease_zoom(ZOOM_FACTOR, ft);
  }
  else {
    arcball.increase_zoom(ZOOM_FACTOR, ft);
  }
}

void
PlayerPlayingGameBehavior::mouse_motion(MouseMotionEvent&& mme)
{
  auto& state        = mme.game_state;
  auto& camera       = mme.camera;
  auto const& motion = mme.motion;
  auto& player       = mme.player;
  auto const& ft     = mme.frame_time;

  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& ms     = es.device_states.mouse;
  auto& ui     = es.ui_state.debug;

  // convert from int to floating-point value
  float const xrel = motion.xrel, yrel = motion.yrel;
  if (camera.is_firstperson()) {
    fps_mousemove(player, camera, player, ms.first_person, xrel, yrel, ft);
  }
  else if (camera.is_thirdperson()) {
    thirdperson_mousemove(player, camera, ms.current, ms.third_person, xrel, yrel, ft);
  }
  else {
    LOG_ERROR("MouseMotion not implemented for this camera mode");
  }
}

void
PlayerPlayingGameBehavior::keyup(KeyEvent&& ke)
{
  auto& state       = ke.game_state;
  auto const& event = ke.event;
  auto& es          = state.engine_state;
  auto& logger      = es.logger;

  switch (event.key.keysym.sym) {
    break;
  }
}

void
PlayerPlayingGameBehavior::keydown(KeyEvent &&ke)
{
  auto& state       = ke.game_state;
  auto& camera      = ke.camera;
  auto const& event = ke.event;
  auto const& ft    = ke.frame_time;
  auto& player      = ke.player;

  auto& es         = state.engine_state;
  auto& logger     = es.logger;
  auto& uistate    = es.ui_state;
  auto& player_wo  = player.world_object();

  auto& lm             = state.level_manager;
  auto& active         = lm.active();
  auto& registry       = active.registry;
  auto& ldata          = active.level_data;

  auto& nbt = ldata.nearby_targets;

  auto& debug = uistate.debug;
  auto& ingame = uistate.ingame;
  auto& chat_state = ingame.chat_state;

  auto const rotate_player = [&](float const angle, auto const axis) {
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
      player.try_pickup_nearby_item(logger, registry, ft);
    }
    break;
  case SDLK_q:
    break;
  case SDLK_F11:
    uistate.draw_debug_ui ^= true;
    break;
  case SDLK_t:
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
    auto const selected_opt = nbt.selected();

    // Toggle the state trackerwhether or not the player is attacking
    // AND
    // If the player has an entity selected, try and attack it.
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
    break;
  case SDLK_KP_MINUS:
    break;
  case SDLK_LEFT:
    rotate_player(90.0f, EulerAxis::Y);
    break;
  case SDLK_RIGHT:
    rotate_player(-90.0f, EulerAxis::Y);
    break;
  }
}

void
PlayerPlayingGameBehavior::process_mouse_state(MouseAndKeyboardArgs &&mk)
{
  auto& state        = mk.game_state;
  auto& camera       = mk.camera;
  auto& player       = mk.player;
  auto const& ft     = mk.frame_time;

  auto& es     = state.engine_state;
  auto& logger = es.logger;
  auto& mss    = es.device_states.mouse;

  auto& ms_now  = mss.current;
  auto& ms_prev = mss.previous;

  bool const both_yes_now = ms_now.both_pressed();
  bool const both_yes_prev = ms_prev.both_pressed();
  bool const both_not_prev = !both_yes_prev;

  auto& lm     = state.level_manager;
  auto& registry  = lm.active().registry;

  auto& movement = es.movement_state;
  if (both_yes_now) {
    player.rotate_to_match_camera_rotation(camera);
    movement.mouse_forward = player.eye_forward();
  }
  else {
    movement.mouse_forward = ZERO;
  }
}

void
PlayerPlayingGameBehavior::process_keyboard_state(MouseAndKeyboardArgs &&mk)
{
  auto& state        = mk.game_state;
  auto& camera       = mk.camera;
  auto& player       = mk.player;
  auto const& ft     = mk.frame_time;

  // continual keypress responses procesed here
  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);

  auto& es       = state.engine_state;
  auto& logger = es.logger;
  auto& lm       = state.level_manager;
  auto& zs       = lm.active();
  auto& ldata    = zs.level_data;
  auto& registry = zs.registry;

  auto& movement = es.movement_state;

  movement.forward = keystate[SDL_SCANCODE_W]
    ? player.eye_forward()
    : ZERO;
  movement.backward = keystate[SDL_SCANCODE_S]
    ? player.eye_backward()
    : ZERO;

  movement.left = keystate[SDL_SCANCODE_A]
    ? player.eye_left()
    : ZERO;

  movement.right = keystate[SDL_SCANCODE_D]
    ? player.eye_right()
    : ZERO;
}

void
PlayerPlayingGameBehavior::process_controller_state(ControllerArgs&& ca)
{
  auto& state             = ca.game_state;
  auto const& controllers = ca.controllers;
  auto& camera            = ca.camera;
  auto& player            = ca.player;
  auto& player_wo         = player.world_object();
  auto const& ft          = ca.frame_time;

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

  auto constexpr THRESHOLD  = 0.4f;
  auto const less_threshold = [&](auto const& v) { return v <= 0 && (v <= AXIS_MIN * THRESHOLD); };
  auto const greater_threshold = [](auto const& v) {
    return v >= 0 && (v >= AXIS_MAX * THRESHOLD);
  };

  auto& movement = es.movement_state;
  auto const axis_left_x = c.axis_left_x();

  if (less_threshold(axis_left_x)) {
    movement.left = player_wo.eye_left();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.left = ZERO;
  }
  if (greater_threshold(axis_left_x)) {
    movement.right = player_wo.eye_right();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.right = ZERO;
  }
  auto const axis_left_y = c.axis_left_y();
  if (less_threshold(axis_left_y)) {
    movement.forward = player_wo.eye_forward();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.forward = ZERO;
  }
  if (greater_threshold(axis_left_y)) {
    movement.backward = player_wo.eye_backward();
    player_wo.rotate_to_match_camera_rotation(camera);
  }
  else {
    movement.backward = ZERO;
  }

  {
    auto const& ds = es.device_states.controller;
    auto const& sens = camera.is_firstperson()
      ? ds.first_person
      : ds.third_person;
    {
      auto const axis_right_x = c.axis_right_x();
      if (less_threshold(axis_right_x)) {
        LOG_ERROR("LT");
        camera.rotate(axis_right_x, 0.0, sens, ft);
        player_wo.rotate_to_match_camera_rotation(camera);
      }
      if (greater_threshold(axis_right_x)) {
        LOG_ERROR("GT");
        camera.rotate(axis_right_x, 0.0, sens, ft);
        player_wo.rotate_to_match_camera_rotation(camera);
      }
    }
    {
      auto const right_axis_y = c.axis_right_y();
      if (less_threshold(right_axis_y)) {
        camera.rotate(0.0, right_axis_y, sens, ft);
      }
      if (greater_threshold(right_axis_y)) {
        camera.rotate(0.0, right_axis_y, sens, ft);
      }
    }
  }
  if (c.button_a()) {
    LOG_DEBUG("BUTTON A\n");
    player.try_pickup_nearby_item(logger, registry, ft);
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

/////////////////////////////////////////////////////gfx_window_sdl///////////////////////////////////////////////
// TerminalOnlyBehavior
void
TerminalOnlyBehavior::mousebutton_down(MouseButtonEvent&&)
{
  std::cerr << "mousebutton down\n";
}

void
TerminalOnlyBehavior::mousebutton_up(MouseButtonEvent&&)
{
  std::cerr << "mousebutton up\n";
}
void
TerminalOnlyBehavior::mouse_motion(MouseMotionEvent&&)
{
  std::cerr << "mouse motion\n";
}

void
TerminalOnlyBehavior::mouse_wheel(MouseWheelEvent&&)
{
  std::cerr << "mouse wheel\n";
}

void
TerminalOnlyBehavior::keydown(KeyEvent &&)
{
  std::cerr << "keydown\n";
}

void
TerminalOnlyBehavior::keyup(KeyEvent&&)
{
  std::cerr << "keyup\n";
}

void
TerminalOnlyBehavior::process_mouse_state(MouseAndKeyboardArgs&&)
{
  std::cerr << "processing mouse state\n";
}

void
TerminalOnlyBehavior::process_keyboard_state(MouseAndKeyboardArgs&&)
{
  std::cerr << "processing keyboard state\n";
}

void
TerminalOnlyBehavior::process_controller_state(ControllerArgs&&)
{
  std::cerr << "processing controller state\n";
}

} // namespace boomhs