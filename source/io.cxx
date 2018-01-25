#include <boomhs/io.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>
#include <boomhs/zone.hpp>
#include <stlw/log.hpp>

#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

namespace {
using namespace boomhs;

inline bool is_quit_event(SDL_Event &event)
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

//glm::vec2
//getNormalizedCoords(float const x, float const y, float const width, float const height)
//{
  //float const ndc_x = 2.0 * x/width - 1.0;
  //float const ndc_y = 1.0 - 2.0 * y/height; // invert Y axis
  //return glm::vec2{ndc_x, ndc_y};
//}

glm::vec3
calculateMouseRay(Camera const& camera, int const mouse_x, int const mouse_y, window::Dimensions const& dimensions)
{
  glm::vec4 const viewport = glm::vec4(dimensions.x, dimensions.y, dimensions.w, dimensions.h);
  glm::mat4 const modelview = camera.view_matrix();
  glm::mat4 const projection = camera.projection_matrix();
  float const height = dimensions.h;
  std::cerr << "viewport: '" << glm::to_string(viewport) << "'\n";

  float z = 0.0;
  glm::vec3 screenPos = glm::vec3(mouse_x, dimensions.h - mouse_y - 1.0f, z);
  std::cerr << "mouse clickpos: xyz: '" << glm::to_string(screenPos) << "'\n";

  glm::vec3 const worldPos = glm::unProject(screenPos, modelview, projection, viewport);
  std::cerr << "calculated worldpos: xyz: '" << glm::to_string(worldPos) << "'\n";
  return worldPos;
}

bool
process_event(GameState &state, SDL_Event &event)
{
  auto &engine_state = state.engine_state;
  stlw::Logger &logger = engine_state.logger;
  float constexpr MOVE_DISTANCE = 1.0f;
  float constexpr SCALE_FACTOR = 0.20f;
  float constexpr ANGLE = 60.0f;

  ZoneManager zm{state.zone_states};
  auto &active = zm.active();
  auto &camera = active.camera;
  auto &player = active.player;

  auto const sf = [](float const f) { return (f > 1.0f) ? (1.0f + f) : (1.0f - f); };

  auto &ui_state = engine_state.ui_state;
  if (ui_state.block_input) {
    return is_quit_event(event);
  }

  auto &tilemap_state = engine_state.tilemap_state;
  auto &mouse_state = engine_state.mouse_state;
  auto const move_player = [&](glm::vec3 (WorldObject::*fn)() const) {
    auto const player_pos = player.tilemap_position();
    glm::vec3 const move_vec = (player.*fn)();

    ZoneManager zm{state.zone_states};
    auto const& tilemap = zm.active().tilemap;

    auto const [x, y, z] = tilemap.dimensions();
    auto const new_pos = player_pos + move_vec;
    bool const out_of_bounds = new_pos.x > x || new_pos.y > y || new_pos.z > z
      || new_pos.x < 0 || new_pos.y < 0 || new_pos.z < 0;
    if (out_of_bounds) {
      return;
    }
    auto const& new_tile = tilemap.data(player_pos + move_vec);
    if (!engine_state.player_collision) {
      player.move(MOVE_DISTANCE, move_vec);
      tilemap_state.recompute = true;
    } else if (!new_tile.is_wall) {
      player.move(MOVE_DISTANCE, move_vec);
      tilemap_state.recompute = true;
    }
  };
  switch (event.type) {
  case SDL_MOUSEMOTION: {

    // If the user pressed enter, don't move the camera based on mouse movements.
    if (ui_state.enter_pressed) {
      break;
    }
    add_from_event(engine_state.mouse_data, event);

    bool const left = event.motion.state & SDL_BUTTON_LMASK;
    bool const right = event.motion.state & SDL_BUTTON_RMASK;

    auto const rot_player = [&]() {
      float const angle = event.motion.xrel > 0 ? 1.0 : -1.0f;
      player.rotate(angle, opengl::Y_UNIT_VECTOR);
      tilemap_state.recompute = true;
    };
    auto const rot_camera = [&]() {
      camera.rotate(logger, ui_state, engine_state.mouse_data);
    };
    if (right) {
      rot_player();
      //rot_camera();
    } else if (left) {
      rot_camera();
    }
    break;
  }
  case SDL_MOUSEWHEEL: {
    LOG_TRACE("mouse wheel event detected.");
    float constexpr ZOOM_FACTOR = 2.0f;
    if (event.wheel.y > 0) {
      camera.zoom(1.0f / ZOOM_FACTOR);
    } else {
      camera.zoom(ZOOM_FACTOR);
    }
    break;
  }
  case SDL_MOUSEBUTTONDOWN:
  {
    auto const& button = event.button.button;
    if (button == SDL_BUTTON_RIGHT) {
      mouse_state.right_pressed = true;
    }
    else if (button == SDL_BUTTON_LEFT) {
      mouse_state.left_pressed = true;
    }
    if (mouse_state.left_pressed && mouse_state.right_pressed) {
      camera.rotate_behind_player(logger, player);
    }

    LOG_ERROR("toggling mouse up/down (pitch) lock");
    engine_state.mouse_data.pitch_lock ^= true;
    break;
  }
  case SDL_MOUSEBUTTONUP:
  {
    auto const& button = event.button.button;
    if (button == SDL_BUTTON_RIGHT) {
      mouse_state.right_pressed = false;
    }
    else if (button == SDL_BUTTON_LEFT) {
      mouse_state.left_pressed = false;
    }
    break;
  }
  case SDL_KEYDOWN: {
    auto const rotate_player = [&](float const angle, glm::vec3 const& axis)
    {
      player.rotate(angle, axis);
      tilemap_state.recompute = true;
    };
    switch (event.key.keysym.sym) {
    case SDLK_w: {
      move_player(&WorldObject::forward_vector);
      break;
    }
    case SDLK_s: {
      move_player(&WorldObject::backward_vector);
      break;
    }
    case SDLK_a: {
      move_player(&WorldObject::left_vector);
      break;
    }
    case SDLK_d: {
      move_player(&WorldObject::right_vector);
      break;
    }
    case SDLK_q: {
      move_player(&WorldObject::up_vector);
      break;
    }
    case SDLK_e: {
      move_player(&WorldObject::down_vector);
      break;
    }
    case SDLK_LEFT: {
      rotate_player(-90.0f, opengl::Y_UNIT_VECTOR);
      break;
    }
    case SDLK_RIGHT: {
      rotate_player(90.0f, opengl::Y_UNIT_VECTOR);
      break;
    }
    case SDLK_t: {
      // invert
      //state.camera.toggle_mode();
      break;
    }
    // scaling
    case SDLK_KP_PLUS: {
    case SDLK_o:
      camera.rotate_behind_player(logger, player);
      //et.scale_entities(sf(SCALE_FACTOR));
      break;
    }
    case SDLK_KP_MINUS: {
    case SDLK_p:
      {
        // 1) Convert mouse location to
        // 3d normalised device coordinates
        int const mouse_x = engine_state.mouse_data.current.x, mouse_y = engine_state.mouse_data.current.y;
        auto const width = engine_state.dimensions.w, height = engine_state.dimensions.h;
        float const x = (2.0f * mouse_x) / width - 1.0f;
        float const y = 1.0f - (2.0f * mouse_y) / height;
        float const z = 1.0f;
        glm::vec3 const ray_nds{x, y, z};

        // homongonize coordinates
        glm::vec4 const ray_clip{ray_nds.x, ray_nds.y, -1.0, 1.0};
        glm::vec4 ray_eye = glm::inverse(camera.projection_matrix()) * ray_clip;
        ray_eye.z = -1.0f;
        ray_eye.w = 0.0f;

        glm::vec3 const ray_wor = glm::normalize(glm::vec3{glm::inverse(camera.view_matrix()) * ray_eye});
        //std::cerr << "mouse: '" << std::to_string(mouse_x) << "', '" << std::to_string(mouse_y) << "'\n";
        //std::cerr << "ray_wor: '" << glm::to_string(ray_wor) << "'\n";

        glm::vec3 const ray_dir = ray_eye;
        glm::vec3 const ray_origin = camera.world_position();
        glm::vec3 const plane_origin{0, 0, 0};
        glm::vec3 const plane_normal{0, -1, 0};

        float distance = 0.0f;
        //bool intersects = glm::intersectRayPlane(ray_origin, ray_dir, plane_origin, plane_normal, distance);
        //std::cerr << "intersects: '" << intersects << "', distance: '" << distance << "'\n";

        auto const ray = calculateMouseRay(camera, mouse_x, mouse_y, engine_state.dimensions);
      }
      break;
    }
    // z-rotation
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
    case SDLK_RETURN: {
      // Toggle state
      auto &ep = ui_state.enter_pressed;
      ep ^= true;
      break;
    }
    }
  }
  }
  return is_quit_event(event);
}

} // ns anon

namespace boomhs
{

void
IO::process(GameState &state, SDL_Event &event)
{
  auto &engine_state = state.engine_state;

  engine_state.LOG_TRACE("IO::process(data, state)");

  //auto et = ::game::entity_factory::make_transformer(state.logger, data);
  while ((!engine_state.quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);

    auto &imgui = engine_state.imgui;
    if (!imgui.WantCaptureMouse && !imgui.WantCaptureKeyboard) {
      engine_state.quit = process_event(state, event);
    }
  }
}

} // ns boomhs
