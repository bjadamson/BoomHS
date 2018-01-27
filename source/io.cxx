#include <boomhs/io.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/zone.hpp>
#include <stlw/log.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <glm/glm.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

float constexpr MOVE_DISTANCE = 1.0f;
float constexpr SCALE_FACTOR = 0.20f;
float constexpr ZOOM_FACTOR = 2.0f;

namespace {
using namespace boomhs;
using namespace window;

glm::vec3
calculate_mouse_worldpos(Camera const& camera, WorldObject const& player, int const mouse_x,
    int const mouse_y, Dimensions const& dimensions)
{
  auto const& a = camera.perspective_ref();
  glm::vec4 const viewport = glm::vec4(0, 0, 1024, 768);
  glm::mat4 const view = camera.view_matrix();
  glm::mat4 const projection = camera.projection_matrix();
  float const height = dimensions.h;

  // Calculate the view-projection matrix.
  glm::mat4 const transform = projection * view;

  // Calculate the intersection of the mouse ray with the near (z=0) and far (z=1) planes.
  glm::vec3 const near = glm::unProject(glm::vec3{mouse_x, dimensions.h - mouse_y, 0}, glm::mat4(), transform, viewport);
  glm::vec3 const far = glm::unProject(glm::vec3{mouse_x, dimensions.h - mouse_y, 1}, glm::mat4(), transform, viewport);

  auto const z = 0.0f;
  glm::vec3 const world_pos = glm::mix(near, far, ((z - near.z) / (far.z - near.z)));

  //float const Z_PLANE = 0.0;
  //assert(768 == dimensions.h);
  //glm::vec3 screen_pos = glm::vec3(mouse_x, (dimensions.h - mouse_y), Z_PLANE);
  //std::cerr << "mouse clickpos: xyz: '" << glm::to_string(screen_pos) << "'\n";

  //glm::vec3 const world_pos = glm::unProject(screen_pos, view, projection, viewport);
  std::cerr << "calculated worldpos: xyz: '" << glm::to_string(world_pos) << "'\n";
  return world_pos;
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
process_mousemotion(GameState &state, SDL_MouseMotionEvent const& motion, float const delta_time)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;
  auto &ms = es.mouse_state;
  auto &ts = es.tilemap_state;
  auto &ui = es.ui_state;

  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &player = active.player;
  auto &camera = active.camera;

  auto const xrel = motion.xrel;
  auto const yrel = motion.yrel;

  if (ms.both_pressed()) {
    player.rotate_to_match_camera_rotation(camera);
  }
    if (ms.left_pressed) {
      auto const& sens = ms.sensitivity;
      float const dx = sens.x * xrel;
      float const dy = sens.y * yrel;
      glm::vec2 const delta{dx, dy};
      camera.rotate(logger, ui, delta);
    }
    if (ms.right_pressed) {
      float const angle = xrel > 0 ? 1.0 : -1.0f;
      player.rotate(angle, opengl::Y_UNIT_VECTOR);
    }
}

void
process_mousebutton_down(GameState &state, SDL_MouseButtonEvent const& event, float const delta_time)
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
    auto &active = zm.active();
    auto &player = active.player;
    auto &camera = active.camera;

    player.rotate_to_match_camera_rotation(camera);
  }
}

void
process_mousebutton_up(GameState &state, SDL_MouseButtonEvent const& event, float const delta_time)
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
process_keyup(GameState &state, SDL_Event const& event, float const delta_time)
{
}

void
process_keydown(GameState &state, SDL_Event const& event, float const delta_time)
{
  auto &es = state.engine_state;
  auto &ui = es.ui_state;
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
        //std::cerr << "ray_wor: '" << glm::to_string(ray_wor) << "'\n";

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
  }
}

void
process_mousewheel(GameState &state, SDL_MouseWheelEvent const& wheel, float const delta_time)
{
  auto &logger = state.engine_state.logger;
  LOG_TRACE("mouse wheel event detected.");

  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &camera = active.camera;
  if (wheel.y > 0) {
    camera.zoom(1.0f / ZOOM_FACTOR);
  } else {
    camera.zoom(ZOOM_FACTOR);
  }
}

void
process_keystate(GameState &state, float const time_delta)
{
  // continual keypress responses procesed here
  uint8_t const* keystate = SDL_GetKeyboardState(nullptr);
  assert(keystate);

  auto &es = state.engine_state;
  auto &ts = es.tilemap_state;
  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &player = active.player;

  if (keystate[SDL_SCANCODE_W]) {
    move_ontilemap(state, MOVE_DISTANCE, &WorldObject::forward_vector, player);
  }
  if (keystate[SDL_SCANCODE_S]) {
    move_ontilemap(state, MOVE_DISTANCE, &WorldObject::backward_vector, player);
  }
  if (keystate[SDL_SCANCODE_A]) {
    move_ontilemap(state, MOVE_DISTANCE, &WorldObject::left_vector, player);
  }
  if (keystate[SDL_SCANCODE_D]) {
    move_ontilemap(state, MOVE_DISTANCE, &WorldObject::right_vector, player);
  }
  if (keystate[SDL_SCANCODE_Q]) {
    move_ontilemap(state, MOVE_DISTANCE, &WorldObject::up_vector, player);
  }
  if (keystate[SDL_SCANCODE_E]) {
    move_ontilemap(state, MOVE_DISTANCE, &WorldObject::down_vector, player);
  }
  auto const rotate_player = [&](float const angle, glm::vec3 const& axis) {
    player.rotate(angle, axis);
    ts.recompute = true;
  };
  if (keystate[SDL_SCANCODE_LEFT]) {
    rotate_player(-90.0f, opengl::Y_UNIT_VECTOR);
  }
  if (keystate[SDL_SCANCODE_RIGHT]) {
    rotate_player(90.0f, opengl::Y_UNIT_VECTOR);
  }
}

bool
process_event(GameState &state, SDL_Event &event, float const delta_time)
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
      process_mousebutton_down(state, event.button, delta_time);
      break;
    case SDL_MOUSEBUTTONUP:
      process_mousebutton_up(state, event.button, delta_time);
      break;
  case SDL_MOUSEMOTION:
      process_mousemotion(state, event.motion, delta_time);
      break;
  case SDL_MOUSEWHEEL:
      process_mousewheel(state, event.wheel, delta_time);
      break;
  case SDL_KEYDOWN:
      process_keydown(state, event, delta_time);
      break;
  case SDL_KEYUP:
      process_keyup(state, event, delta_time);
      break;
  }
  return is_quit_event(event);
}

} // ns anon

namespace boomhs
{

void
IO::process(GameState &state, SDL_Event &event, float const delta_time)
{
  auto &es = state.engine_state;
  auto &logger = es.logger;

  LOG_TRACE("IO::process(data, state)");
  while ((!es.quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);

    auto &imgui = es.imgui;
    if (!imgui.WantCaptureMouse && !imgui.WantCaptureKeyboard) {
      es.quit = process_event(state, event, delta_time);
    }
  }
  process_keystate(state, delta_time);
}

} // ns boomhs
