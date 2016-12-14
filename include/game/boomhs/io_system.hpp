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

  template<typename L, typename TData, typename S>
  bool process_event(L &logger, TData data, SDL_Event &event, S &state) const
  {
    float constexpr PAN_DISTANCE = 0.3f;
    float constexpr MOVE_DISTANCE = 0.1f;
    float constexpr SF = 0.20f; // scale-factor

    float constexpr ANGLE = 0.2f;
    float constexpr SCALE_FACTOR = 0.1f;

    glm::mat4 &view = state.view;
    glm::mat4 &projection = state.projection;
    auto const make_scalev = [](float const f) { return glm::vec3(1.0f + f, 1.0f + f, 1.0f + f); };

    switch (event.type) {
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      // translation
      case SDLK_UP: {
        view = glm::translate(view, glm::vec3(0.0f, PAN_DISTANCE, 0.0f));
        break;
      }
      case SDLK_DOWN: {
        view = glm::translate(view, glm::vec3(0.0f, -PAN_DISTANCE, 0.0f));
        break;
      }
      case SDLK_LEFT: {
        view = glm::translate(view, glm::vec3(-PAN_DISTANCE, 0.0f, 0.0f));
        break;
      }
      case SDLK_RIGHT: {
        view = glm::translate(view, glm::vec3(PAN_DISTANCE, 0.0f, 0.0f));
        break;
      }
      // scaling
      case SDLK_KP_PLUS: {
      case SDLK_o:
        auto constexpr SCALE_VECTOR = glm::vec3{1.0f + SF, 1.0f + SF, 1.0f + SF};
        scale_entities(logger, data, SCALE_VECTOR);
        break;
      }
      case SDLK_PLUS: {
        view = glm::scale(view, make_scalev(SCALE_FACTOR));
        break;
      }
      case SDLK_KP_MINUS: {
      case SDLK_p:
        auto constexpr SCALE_VECTOR = glm::vec3{1.0f - SF, 1.0f - SF, 1.0f - SF};
        scale_entities(logger, data, SCALE_VECTOR);
        break;
      }
      case SDLK_MINUS: {
        view = glm::scale(view, make_scalev(-SCALE_FACTOR));
        break;
      }
      // z-rotation
      case SDLK_1: {
        view = glm::rotate(view, ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      case SDLK_j: {
        auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};
        rotate_entities(logger, data, ANGLE, ROTATION_VECTOR);
        break;
      }
      case SDLK_2: {
        view = glm::rotate(view, -ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      case SDLK_k: {
        auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 0.0f, -1.0f};
        rotate_entities(logger, data, ANGLE, ROTATION_VECTOR);
        break;
      }
      // y-rotation
      case SDLK_3: {
        view = glm::rotate(view, ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      case SDLK_u: {
        auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
        rotate_entities(logger, data, ANGLE, ROTATION_VECTOR);
        break;
      }
      case SDLK_4: {
        view = glm::rotate(view, -ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      case SDLK_i: {
        auto constexpr ROTATION_VECTOR = glm::vec3{0.0f, -1.0f, 0.0f};
        rotate_entities(logger, data, ANGLE, ROTATION_VECTOR);
        break;
      }
      // x-rotation
      case SDLK_5: {
        view = glm::rotate(view, ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      case SDLK_n: {
        auto constexpr ROTATION_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
        rotate_entities(logger, data, -ANGLE, ROTATION_VECTOR);
        break;
      }
      case SDLK_6: {
        view = glm::rotate(view, -ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      case SDLK_m: {
        auto constexpr ROTATION_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
        rotate_entities(logger, data, ANGLE, ROTATION_VECTOR);
        break;
      }
      // transform controls
      case SDLK_w: {
          auto constexpr UP_VECTOR = glm::vec3{0.0f, MOVE_DISTANCE, 0.0f};
          move_entities(logger, data, UP_VECTOR);
        break;
      }
      case SDLK_s: {
          auto constexpr UP_VECTOR = glm::vec3{0.0f, -MOVE_DISTANCE, 0.0f};
          move_entities(logger, data, UP_VECTOR);
        break;
      }
      case SDLK_a: {
          auto constexpr UP_VECTOR = glm::vec3{-MOVE_DISTANCE, 0.0f, 0.0f};
          move_entities(logger, data, UP_VECTOR);
        break;
      }
      case SDLK_d: {
          auto constexpr UP_VECTOR = glm::vec3{MOVE_DISTANCE, 0.0f, 0.0f};
          move_entities(logger, data, UP_VECTOR);
        break;
      }
      case SDLK_KP_1: {
        projection = glm::rotate(projection, ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      case SDLK_KP_2: {
        projection = glm::rotate(projection, -ANGLE, glm::vec3(1.0f, 0.0f, 0.0f));
        break;
      }
      case SDLK_KP_3: {
        projection = glm::rotate(projection, ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      case SDLK_KP_4: {
        projection = glm::rotate(projection, -ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));
        break;
      }
      case SDLK_KP_5: {
        projection = glm::rotate(projection, ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      case SDLK_KP_6: {
        projection = glm::rotate(projection, -ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
        break;
      }
      }
    }
    }
    return is_quit_event(event);
  }

  template<typename L, typename TData, typename FN>
  void for_entity(L &logger, TData &data, ecst::entity_id const eid, char const* action,
      FN const& fn) const
  {
    logger.trace(fmt::sprintf("'%s' entity eid '%d'", action, eid));
    fn();
  }

  template<typename L, typename TData>
  void move_entity(L &logger, TData &data, ecst::entity_id const eid, glm::vec3 const& distance) const
  {
    auto const fn = [&]() {
      auto &m = data.get(ct::model, eid);
      m.translation = glm::translate(m.translation, distance);
    };
    for_entity(logger, data, eid, "moving", fn);
  }

  template<typename L, typename TData>
  void rotate_entity(L &logger, TData &data, ecst::entity_id const eid, float const angle,
      glm::vec3 const& axis) const
  {
    auto const fn = [&]() {
      //auto const& wc = data.get(ct::world_coordinate, eid);
      auto &m = data.get(ct::model, eid);
      auto const q = glm::angleAxis(glm::degrees(angle), axis);
      m.rotation = q * m.rotation;
    };
    for_entity(logger, data, eid, "rotating", fn);
  }

  template<typename L, typename TData>
  void scale_entity(L &logger, TData &data, ecst::entity_id const eid, glm::vec3 const& sf) const
  {
    auto const fn = [&]() {
      auto &m = data.get(ct::model, eid);
      m.scale = glm::scale(m.scale, sf);
    };
    for_entity(logger, data, eid, "scaling", fn);
  }

  template<typename L, typename TData>
  void move_entities(L &logger, TData &data, glm::vec3 const& direction) const
  {
    data.for_entities([&](auto const eid) {
        move_entity(logger, data, eid, direction);
    });
  }

  template<typename L, typename TData>
  void rotate_entities(L &logger, TData &data, float const angle, glm::vec3 const& axis) const
  {
    data.for_entities([&](auto const eid) {
        rotate_entity(logger, data, eid, angle, axis);
    });
  }

  template<typename L, typename TData>
  void scale_entities(L &logger, TData &data, glm::vec3 const& sf) const
  {
    data.for_entities([&](auto const eid) {
        scale_entity(logger, data, eid, sf);
    });
  }

  template <typename TData, typename S>
  void process(TData &data, S &state) const
  {
    state.logger.trace("io_system::process(data, state)");
    SDL_Event event;
    while ((!state.quit) && (0 != SDL_PollEvent(&event))) {
      state.quit = this->process_event(state.logger, data, event, state);
    }
  }
};

} // ns s
