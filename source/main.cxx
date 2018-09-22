#include <boomhs/boomhs.hpp>
#include <boomhs/camera.hpp>
#include <boomhs/controller.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/frame_time.hpp>
#include <boomhs/io_behavior.hpp>
#include <boomhs/io_sdl.hpp>
#include <boomhs/main_menu.hpp>
#include <boomhs/random.hpp>
#include <boomhs/scene_renderer.hpp>
#include <boomhs/state.hpp>

#include <common/timer.hpp>
#include <common/log.hpp>
#include <common/time.hpp>

#include <gl_sdl/sdl_window.hpp>
#include <opengl/renderer.hpp>

#include <extlibs/imgui.hpp>
#include <extlibs/openal.hpp>

using namespace boomhs;
using namespace boomhs::math;
using namespace common;
using namespace gl_sdl;

namespace
{

void
loop_events(GameState& state, Camera& camera, bool const show_mainmenu, bool& quit,
            FrameTime const& ft)
{
  auto const& fn = show_mainmenu
    ? &main_menu::process_event
    : &IO_SDL::process_event;

  SDL_Event event;
  while ((!quit) && (0 != SDL_PollEvent(&event))) {
    ImGui_ImplSdlGL3_ProcessEvent(&event);
    fn(SDLEventProcessArgs{state, event, camera, ft});
    quit |= event.type == SDL_QUIT;
  }
}

void
loop(Engine& engine, GameState& state, StaticRenderers& renderers, RNG& rng, Camera& camera,
     FrameTime const& ft)
{
  auto& es     = state.engine_state;
  auto& logger = es.logger;

  auto& window = engine.window;

  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(window.raw());

  loop_events(state, camera, es.main_menu.show, es.quit, ft);

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
  auto& es     = state.engine_state;
  auto& logger = es.logger;

  Timer timer;
  FrameCounter fcounter;

  auto& zs = state.level_manager.active();
  auto renderers = make_static_renderers(es, zs);
  while (!es.quit) {
    auto const ft = FrameTime::from_timer(timer);
    loop(engine, state, renderers, rng, camera, ft);

    if ((fcounter.frames_counted % 60 == 0)) {
      es.time.add_hours(1);
    }
    timer.update();
    fcounter.update();
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
  auto constexpr FAR    = 100.0f;
  auto const dimensions = engine.dimensions();

  // clang-format off
  Frustum const frustum{
    dimensions.float_left(),
    dimensions.float_right(),
    dimensions.float_bottom(),
    dimensions.float_top(),
    NEAR,
    FAR};
  // clang-format on

  auto& io = ImGui::GetIO();

  // Disable ImGui's Software cursor
  //
  // Instead we'll use the system hardware cursor (using SDL).
  io.MouseDrawCursor = false;

  EngineState es{logger, *al_device, io, dimensions, frustum};

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const PERS_FORWARD = -constants::Z_UNIT_VECTOR;
  auto constexpr PERS_UP  =  constants::Y_UNIT_VECTOR;
  WorldOrientation const pers_wo{PERS_FORWARD, PERS_UP};

  auto const ORTHO_FORWARD = -constants::Y_UNIT_VECTOR;
  auto constexpr ORTHO_UP  =  constants::Z_UNIT_VECTOR;
  WorldOrientation const ortho_wo{ORTHO_FORWARD, ORTHO_UP};
  auto                   camera = Camera::make_default(dimensions, pers_wo, ortho_wo);

  RNG rng;
  auto gs = TRY_MOVEOUT(boomhs::init(engine, es, camera, rng));

  // Start game in a timed loop
  timed_game_loop(engine, gs, camera, rng);

  // Game has finished
  LOG_TRACE("game loop finished.");
  return OK_NONE;
}

} // namespace

int
main(int argc, char* argv[])
{
  auto const time_result = Time::get_time_now();
  if (!time_result) {
    return EXIT_FAILURE;
  }
  std::string const log_name = time_result.unwrap() + "-BoomHS.log";
  auto              logger   = common::LogFactory::make_default(log_name.c_str());

  bool constexpr FULLSCREEN = false;
  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  LOG_DEBUG("Initializing OpenGL context and SDL window.");
  TRY_OR_ELSE_RETURN(auto sdl_gl, SDLGlobalContext::create(logger), on_error);
  TRY_OR_ELSE_RETURN(auto window, sdl_gl->make_window(logger, "BoomHS", FULLSCREEN, 1024, 768), on_error);
  TRY_OR_ELSE_RETURN(auto controller, SDLControllers::find_attached_controllers(logger), on_error);
  Engine engine{MOVE(window), MOVE(controller)};

  LOG_DEBUG("Starting game loop");
  TRY_OR_ELSE_RETURN(auto _, start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
