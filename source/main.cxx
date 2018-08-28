#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/controller.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/io_behavior.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/scene_renderer.hpp>
#include <boomhs/state.hpp>

#include <common/log.hpp>
#include <boomhs/random.hpp>

#include <window/sdl_window.hpp>
#include <boomhs/clock.hpp>

#include <extlibs/imgui.hpp>
#include <extlibs/openal.hpp>

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
loop_events(SDLEventProcessArgs&& epa, FN const& fn)
{
  auto& es    = epa.game_state.engine_state;
  auto& event = epa.event;

  while ((!es.quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);
    fn(MOVE(epa));
  }
}

void
loop(Engine& engine, GameState& state, RNG& rng, Camera& camera,
     FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;

  // Reset Imgui for next game frame.
  auto& window = engine.window;
  ImGui_ImplSdlGL3_NewFrame(window.raw());

  auto const& event_fn = es.main_menu.show ? &main_menu::process_event : &IO_SDL::process_event;

  SDL_Event event;
  loop_events(SDLEventProcessArgs{state, event, camera, ft}, event_fn);
  es.quit |= is_quit_event(event);

  static auto renderers = make_static_renderers(es, state.level_manager);
  boomhs::game_loop(engine, state, renderers, rng, camera, ft);

  // Render Imgui UI
  ImGui::Render();
  ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(window.raw());
}

void
timed_game_loop(Engine& engine, GameState& state, Camera& camera, RNG& rng)
{
  Clock         clock;
  FrameCounter  counter;

  auto& logger = state.engine_state.logger;
  while (!state.engine_state.quit) {
    auto const ft = clock.frame_time();
    loop(engine, state, rng, camera, ft);

    if ((counter.frames_counted % 60 == 0)) {
      state.engine_state.time.add_hours(1);
    }
    clock.update();
    counter.update(logger, clock);
  }
}

Result<common::none_t, std::string>
start(common::Logger& logger, Engine& engine)
{
  // Initialize GUI library
  auto* imgui_context = ImGui::CreateContext();
  ON_SCOPE_EXIT([&imgui_context]() { ImGui::DestroyContext(imgui_context); });

  ImGui_ImplSdlGL3_Init(engine.window.raw());
  ON_SCOPE_EXIT([]() { ImGui_ImplSdlGL3_Shutdown(); });

  ImGui::StyleColorsDark();

  auto& registries = engine.registries;

  // Initialize opengl
  opengl::render::init(logger);

  // Initialize openAL
  ALCdevice* al_device = alcOpenDevice(nullptr);
  if (!al_device) {
    return Err(fmt::sprintf("Error opening openal device"));
  }
  ON_SCOPE_EXIT([&al_device]() { alcCloseDevice(al_device); });
  ALCcontext* ctx = alcCreateContext(al_device, nullptr);
  if (!ctx) {
    return Err(fmt::sprintf("Error making openal context current"));
  }
  ON_SCOPE_EXIT([&ctx]() {
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
  });
  alcMakeContextCurrent(ctx);

  // Construct game state
  auto constexpr NEAR   = 0.001f;
  auto constexpr FAR    = 10000.0f;
  auto const dimensions = engine.dimensions();
  Frustum const frustum{
    static_cast<float>(dimensions.left()),
    static_cast<float>(dimensions.right()),
    static_cast<float>(dimensions.bottom()),
    static_cast<float>(dimensions.top()),
    NEAR,
    FAR};

  auto& io = ImGui::GetIO();
  EngineState es{logger, *al_device, io, dimensions, frustum};
  auto        camera = Camera::make_default(dimensions);

  RNG rng;
  auto gs = TRY_MOVEOUT(boomhs::init(engine, es, camera, rng));

  // Start game in a timed loop
  timed_game_loop(engine, gs, camera, rng);

  // Game has finished
  LOG_TRACE("game loop finished.");
  return OK_NONE;
}

} // namespace

using WindowResult = Result<SDLWindow, std::string>;
WindowResult
make_window(common::Logger& logger, bool const fullscreen, float const width, float const height)
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
  auto              logger   = common::LogFactory::make_default(log_name.c_str());

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
