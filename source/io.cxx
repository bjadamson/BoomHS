#include <boomhs/io.hpp>
#include <imgui/imgui.hpp>
#include <imgui/imgui_impl_sdl_gl3.h>
#include <stlw/log.hpp>

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
    camera.rotate_to(logger, state.mouse_data);
    break;
  }
  case SDL_MOUSEWHEEL: {
    LOG_TRACE("mouse wheel event detected.");
    if (event.wheel.y > 0) {
      LOG_ERROR("increasing sensitivity by factor 2");
      state.mouse_data.sensitivity.x *= 2.0f;
      state.mouse_data.sensitivity.y *= 2.0f;
    } else {
      LOG_ERROR("decreasing sensitivity by factor 2");
      state.mouse_data.sensitivity.x *= 0.5f;
      state.mouse_data.sensitivity.y *= 0.5f;
    }
    break;
  }
  case SDL_MOUSEBUTTONDOWN:
  {
    LOG_ERROR("toggling mouse up/down (pitch) lock");
    state.mouse_data.pitch_lock = !state.mouse_data.pitch_lock;
    break;
  }
  case SDL_MOUSEBUTTONUP:
  {
    break;
  }
  case SDL_KEYDOWN: {
    switch (event.key.keysym.sym) {
    case SDLK_w: {
      camera.move_forward(MOVE_DISTANCE);
      break;
    }
    case SDLK_s: {
      camera.move_backward(MOVE_DISTANCE);
      break;
    }
    case SDLK_a: {
      camera.move_right(MOVE_DISTANCE);
      break;
    }
    case SDLK_d: {
        camera.move_left(MOVE_DISTANCE);
      break;
    }
    case SDLK_q: {
      camera.move_up(MOVE_DISTANCE);
      break;
    }
    case SDLK_e: {
        camera.move_down(MOVE_DISTANCE);
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
