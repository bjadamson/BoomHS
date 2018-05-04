#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/io.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui_debug.hpp>

#include <stlw/log.hpp>
#include <stlw/random.hpp>

#include <window/controller.hpp>
#include <window/sdl_window.hpp>

#include <extlibs/imgui.hpp>

using namespace boomhs;
using namespace window;

namespace
{

bool
is_quit_event(SDL_Event& event)
{
  bool is_quit = false;

  switch (event.type) {
  case SDL_QUIT: {
    is_quit = true;
    break;
  }
  }
  return is_quit;
}

template <typename FN>
void
loop_events(GameState& state, SDL_Event& event, Camera& camera, FrameTime const& ft, FN const& fn)
{
  auto& es = state.engine_state;
  while ((!es.quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);
    fn(state, event, camera, ft);
  }
}

void
loop(Engine& engine, GameState& state, stlw::float_generator& rng, Camera& camera,
     FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;

  // Reset Imgui for next game frame.
  auto& window = engine.window;
  ImGui_ImplSdlGL3_NewFrame(window.raw());

  auto const& event_fn = es.show_main_menu ? &main_menu::process_event : &IO::process_event;

  SDL_Event event;
  loop_events(state, event, camera, ft, event_fn);
  es.quit |= is_quit_event(event);

  auto& io = es.imgui;
  if (es.show_main_menu) {
    // Enable keyboard shortcuts
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    auto const& size = engine.dimensions();
    main_menu::draw(es, ImVec2(size.w, size.h));
  }
  else {
    // Disable keyboard shortcuts
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    IO::process(state, engine.controllers, camera, ft);
    boomhs::game_loop(engine, state, rng, camera, ft);
  }
  auto& ui_state = es.ui_state;
  if (ui_state.draw_debug_ui) {
    auto& lm = state.level_manager;
    ui_debug::draw(es, lm, window, camera, ft);
  }

  // Render Imgui UI
  ImGui::Render();
  ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(window.raw());
}

void
timed_game_loop(Engine& engine, GameState& state, Camera& camera)
{
  window::Clock         clock;
  window::FrameCounter  counter;
  stlw::float_generator rng;

  auto& logger = state.engine_state.logger;
  while (!state.engine_state.quit) {
    auto const ft = clock.frame_time();
    loop(engine, state, rng, camera, ft);
    clock.update();
    counter.update(logger, clock);
  }
}

Result<stlw::none_t, std::string>
start(stlw::Logger& logger, Engine& engine)
{
  // Initialize GUI library
  auto* imgui_context = ImGui::CreateContext();
  ON_SCOPE_EXIT([&imgui_context]() { ImGui::DestroyContext(imgui_context); });

  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  ImGui::StyleColorsDark();

  auto& registries = engine.registries;

  // Initialize opengl
  auto const dimensions = engine.dimensions();
  boomhs::render::init(logger, dimensions);

  // Configure Imgui
  auto& io           = ImGui::GetIO();
  io.MouseDrawCursor = true;
  io.DisplaySize     = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  // Construct game state
  auto        camera = Camera::make_defaultcamera();
  EngineState es{logger, io, dimensions};
  GameState   gs = TRY_MOVEOUT(boomhs::init(engine, es, camera));

  // Start game in a timed loop
  timed_game_loop(engine, gs, camera);

  // Game has finished
  LOG_TRACE("game loop finished.");
  return OK_NONE;
}

} // namespace

using WindowResult = Result<SDLWindow, std::string>;
WindowResult
make_window(stlw::Logger& logger, bool const fullscreen, float const width, float const height)
{
  // Select windowing library as SDL.
  LOG_DEBUG("Initializing window library globals");
  DO_EFFECT(window::sdl_library::init(logger));

  LOG_DEBUG("Instantiating window instance.");
  return window::sdl_library::make_window(logger, fullscreen, height, width);
}

int
main(int argc, char* argv[])
{
  auto const time_result = Time::get_time_now();
  if (!time_result) {
    return EXIT_FAILURE;
  }
  std::string const log_name = time_result.unwrap() + "-BoomHS.log";
  auto              logger   = stlw::LogFactory::make_default(log_name.c_str());

  LOG_DEBUG("Creating window ...");
  bool constexpr FULLSCREEN = false;

  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };
  TRY_OR_ELSE_RETURN(auto window, make_window(logger, FULLSCREEN, 1024, 768), on_error);
  TRY_OR_ELSE_RETURN(auto controller, SDLControllers::find_attached_controllers(logger), on_error);
  Engine engine{MOVE(window), MOVE(controller)};

  LOG_DEBUG("Starting game loop");
  TRY_OR_ELSE_RETURN(auto _, start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
