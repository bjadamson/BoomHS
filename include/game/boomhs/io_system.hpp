#pragma once
#include <engine/gfx/opengl/glsl.hpp>
#include <engine/window/sdl_window.hpp>

namespace s
{

struct io_system {

  template <typename L>
  bool init(L &logger)
  {
    logger.trace("io_system::init()");
    return true;
  }

  inline static bool is_quit_event(SDL_Event &event)
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
  bool process_event(SDL_Event &event, glm::mat4 &view, glm::mat4 &proj) const
  {
    float constexpr DISTANCE = 0.1f;
    float constexpr ANGLE = 0.2f;
    float constexpr SCALE_FACTOR = 0.1f;

    auto const make_scalev = [](float const f) { return glm::vec3(1.0f + f, 1.0f + f, 1.0f + f); };

    switch (event.type) {
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      // translation
      case SDLK_w: {
        view = glm::translate(view, glm::vec3(0.0f, DISTANCE, 0.0f));
        break;
      }
      case SDLK_s: {
        view = glm::translate(view, glm::vec3(0.0f, -DISTANCE, 0.0f));
        break;
      }
      case SDLK_a: {
        view = glm::translate(view, glm::vec3(-DISTANCE, 0.0f, 0.0f));
        break;
      }
      case SDLK_d: {
        view = glm::translate(view, glm::vec3(DISTANCE, 0.0f, 0.0f));
        break;
      }
      // scaling
      case SDLK_o: {
        view = glm::scale(view, make_scalev(SCALE_FACTOR));
        break;
      }
      case SDLK_p: {
        view = glm::scale(view, make_scalev(-SCALE_FACTOR));
        break;
      }
      // z-rotation
      case SDLK_j: {
        view = glm::rotate(view, ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      case SDLK_k: {
        view = glm::rotate(view, -ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      // y-rotation
      case SDLK_u: {
        view = glm::rotate(view, ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      case SDLK_i: {
        view = glm::rotate(view, -ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      // x-rotation
      case SDLK_n: {
        view = glm::rotate(view, ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      case SDLK_m: {
        view = glm::rotate(view, -ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      }
    }
    }
    return is_quit_event(event);
  }

  template <typename TData, typename S>
  void process(TData &data, S &state)
  {
    state.logger.trace("io_system::process(data, state)");
    SDL_Event event;
    while ((!state.quit) && (0 != SDL_PollEvent(&event))) {
      state.quit = this->process_event(event, state.view, state.projection);
    }
  }
};

} // ns s
