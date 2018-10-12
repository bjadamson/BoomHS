#include <boomhs/camera.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/controller.hpp>
#include <boomhs/engine.hpp>
#include <boomhs/frame.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/io_behavior.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/npc.hpp>
#include <boomhs/player.hpp>
#include <boomhs/state.hpp>
#include <boomhs/world_object.hpp>

#include <gl_sdl/sdl_window.hpp>

#include <common/log.hpp>
#include <boomhs/math.hpp>

#include <extlibs/imgui.hpp>
#include <extlibs/sdl.hpp>
#include <iostream>

using namespace boomhs;
using namespace boomhs::math;
using namespace boomhs::math::constants;
using namespace opengl;
using namespace gl_sdl;

namespace boomhs
{

void
IO_SDL::process_event(SDLEventProcessArgs &&epa)
{
  auto& state    = epa.game_state;
  auto& event    = epa.event;
  auto& camera   = epa.camera;
  auto const& ft = epa.frame_time;

  auto& es     = state.engine_state();
  auto& logger = es.logger;

  auto& lm     = state.level_manager();
  auto& active = lm.active();
  auto& registry = active.registry;

  auto& player = find_player(registry);
  auto& player_wo = player.world_object();

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
    case SDLK_F8:
        es.behaviors.active = &es.behaviors.player_playing_behavior;
        break;
      case SDLK_F9:
        es.behaviors.active = &es.behaviors.terminal_behavior;
        break;

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

  assert(es.behaviors.active);
  auto const& behavior = *es.behaviors.active;
  switch (type) {
  case SDL_MOUSEBUTTONDOWN:
    behavior.on_mouse_down(MouseButtonEvent{state, camera, event.button, player_wo, ft});
    break;
  case SDL_MOUSEBUTTONUP:
    behavior.on_mouse_up(MouseButtonEvent{state, camera, event.button, player_wo, ft});
    break;
  case SDL_MOUSEMOTION:
    behavior.on_mouse_motion(MouseMotionEvent{state, event.motion, camera, player, ft});
    break;
  case SDL_MOUSEWHEEL:
    behavior.on_mouse_wheel(MouseWheelEvent{state, event.wheel, camera, player_wo, ft});
    break;
  case SDL_KEYDOWN:
    behavior.on_key_down(KeyEvent{state, event, camera, player, ft});
    break;
  case SDL_KEYUP:
    behavior.on_key_up(KeyEvent{state, event, camera, player, ft});
    break;
  }
}

void
IO_SDL::read_devices(SDLReadDevicesArgs &&rda)
{
  auto& state             = rda.game_state;
  auto const& controllers = rda.controllers;
  auto& camera            = rda.camera;
  auto const& ft          = rda.frame_time;

  auto& es         = state.engine_state();
  auto& uistate    = es.ui_state;
  auto& ingame     = uistate.ingame;
  auto& chat_state = ingame.chat_state;

  if (chat_state.currently_editing) {
    return;
  }

  auto& zs = state.level_manager().active();
  auto& registry = zs.registry;

  auto& player = find_player(registry);
  auto& player_wo = player.world_object();

  auto& behaviors = es.behaviors;
  assert(behaviors.active);
  auto& behavior = *behaviors.active;
  behavior.update_keyboard(MouseAndKeyboardArgs{state, camera, player_wo, ft});
  behavior.update_mouse(MouseAndKeyboardArgs{state, camera, player_wo, ft});
  if (!es.disable_controller_input) {
    // TODO: using controller and keyboard input at the same time does not work.
    // reason: The controller when it's stick's aren't activated, every frame, set's the same
    // variables to the keyboard controller would use to 0, effectively nullifying any input the
    // keyboard can do.
    //
    // Idea: We could use separate vector's for tracking the controller input, if we want to allow
    // both at the same time (why?).
    behavior.update_controller(ControllerArgs{state, controllers, player, camera, ft});
  }
}

} // namespace boomhs
