#include <boomhs/io.hpp>
#include <boomhs/state.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <stlw/log.hpp>
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

bool process_event(GameState &state, SDL_Event &event)
{
  stlw::Logger &logger = state.logger;
  float constexpr MOVE_DISTANCE = 0.1f;
  float constexpr SCALE_FACTOR = 0.20f;

  float constexpr ANGLE = 60.0f;

  auto &camera = state.camera;
  auto &player = state.player;
  auto const sf = [](float const f) { return (f > 1.0f) ? (1.0f + f) : (1.0f - f); };

  if (state.ui_state.block_input) {
    return is_quit_event(event);
  }

  switch (event.type) {
  case SDL_MOUSEMOTION: {

    // If the user pressed enter, don't move the camera based on mouse movements.
    if (state.ui_state.enter_pressed) {
      break;
    }
    add_from_event(state.mouse_data, event);

    bool const left = event.motion.state & SDL_BUTTON_LMASK;
    bool const right = event.motion.state & SDL_BUTTON_RMASK;
    bool const both = left && right;

    auto const rot_player = [&]() {
      player.rotate(ANGLE, state.mouse_data);
      state.render.tilemap.redraw = true;
    };
    auto const rot_camera = [&]() {
      camera.rotate(logger, state.ui_state, state.mouse_data);
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
      state.mouse.right_pressed = true;
    }
    else if (button == SDL_BUTTON_LEFT) {
      state.mouse.left_pressed = true;
    }

    if (state.mouse.left_pressed && state.mouse.right_pressed) {
      std::cerr << "BOTH BUTTONS PRESSED\n";
      camera.move_behind_player(player);
      state.render.tilemap.redraw = true;
      // rot from player -> camera
      //glm::quat const rot = camera.orientation() * glm::inverse(player.orientation());
      //auto euler = glm::eulerAngles(rot);
      //auto const newrot = glm::rotate(glm::mat4{}, euler.x, opengl::X_UNIT_VECTOR);
      //player.multiply_quat(newrot);
      //player.rotate(
    }
    LOG_ERROR("toggling mouse up/down (pitch) lock");
    state.mouse_data.pitch_lock ^= true;
    break;
  }
  case SDL_MOUSEBUTTONUP:
  {
    auto const& button = event.button.button;
    if (button == SDL_BUTTON_RIGHT) {
      state.mouse.right_pressed = false;
    }
    else if (button == SDL_BUTTON_LEFT) {
      state.mouse.left_pressed = false;
    }
    break;
  }
  case SDL_KEYDOWN: {
    auto const move_player = [&](glm::vec3 (Player::*fn)() const) {
      auto const player_pos = player.tilemap_position();
      glm::vec3 const move_vec = (player.*fn)();

      auto const& new_pos_tile = state.tilemap.data(player_pos + move_vec);
      if (!state.collision.player) {
        player.move(MOVE_DISTANCE, move_vec);
        state.render.tilemap.redraw = true;
      } else if (!new_pos_tile.is_wall) {
        player.move(MOVE_DISTANCE, move_vec);
        state.render.tilemap.redraw = true;
      }
    };
    switch (event.key.keysym.sym) {
    case SDLK_w: {
      move_player(&Player::forward_vector);
      break;
    }
    case SDLK_s: {
      move_player(&Player::backward_vector);
      break;
    }
    case SDLK_a: {
      move_player(&Player::left_vector);
      break;
    }
    case SDLK_d: {
      move_player(&Player::right_vector);
      break;
    }
    case SDLK_q: {
      move_player(&Player::up_vector);
      break;
    }
    case SDLK_e: {
      move_player(&Player::down_vector);
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
      //et.scale_entities(sf(SCALE_FACTOR));
      break;
    }
    case SDLK_KP_MINUS: {
    case SDLK_p:
      //et.scale_entities(sf(-SCALE_FACTOR));
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
      auto &ep = state.ui_state.enter_pressed;
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
  state.LOG_TRACE("IO::process(data, state)");

  //auto et = ::game::entity_factory::make_transformer(state.logger, data);
  while ((!state.quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);

    auto &imgui = state.imgui;
    if (!imgui.WantCaptureMouse && !imgui.WantCaptureKeyboard) {
      state.quit = process_event(state, event);
    }
  }
}

} // ns boomhs
