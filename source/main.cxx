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

#include <gl_sdl/common.hpp>
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
loop(Engine& engine, GameState& gs, RNG& rng, Camera& camera, FrameTime const& ft)
{
  auto& es     = gs.engine_state();
  auto& logger = es.logger;

  auto& window = engine.window;

  // Reset Imgui for next game frame.
  ImGui_ImplSdlGL3_NewFrame(window.raw());

  loop_events(gs, camera, es.main_menu.show, es.quit, ft);
  boomhs::game_loop(engine, gs, rng, camera, ft);

  // Render Imgui UI
  ImGui::Render();
  ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());

  // Update window with OpenGL rendering
  SDL_GL_SwapWindow(window.raw());
}

void
timed_game_loop(Engine& engine, GameState& gs, Camera& camera, RNG& rng)
{
  Timer timer;
  FrameCounter fcounter;

  auto& es = gs.engine_state();
  while (!es.quit) {
    auto const ft = FrameTime::from_timer(timer);
    loop(engine, gs, rng, camera, ft);

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
  OR::init(logger);

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
  auto constexpr FAR    = 5000.0f;
  auto const window_viewport = engine.window_viewport();
  auto const frustum = Frustum::from_rect_and_nearfar(window_viewport.rect(), NEAR, FAR);

  auto& io = ImGui::GetIO();

  // Disable ImGui's Software cursor
  //
  // Instead we'll use the system hardware cursor (using SDL).
  io.MouseDrawCursor = false;

  EngineState es{logger, *al_device, io, frustum};

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const PERS_FORWARD = -constants::Z_UNIT_VECTOR;
  auto constexpr PERS_UP  =  constants::Y_UNIT_VECTOR;
  WorldOrientation const pers_wo{PERS_FORWARD, PERS_UP};

  auto const ORTHO_FORWARD = -constants::Y_UNIT_VECTOR;
  auto constexpr ORTHO_UP  =  constants::Z_UNIT_VECTOR;
  WorldOrientation const ortho_wo{ORTHO_FORWARD, ORTHO_UP};
  auto              camera = Camera::make_default(CameraMode::ThirdPerson, pers_wo, ortho_wo);

  RNG rng;
  auto gs = TRY_MOVEOUT(boomhs::create_gamestate(engine, es, camera, rng));
  boomhs::init_gamestate_inplace(gs, camera);

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
  bool constexpr FULLSCREEN = false;
  char const* TITLE         = "BoomHS";

  auto const time_result = Time::get_time_now();
  if (!time_result) {
    return EXIT_FAILURE;
  }
  std::string const log_name = time_result.unwrap() + "-" + std::string{TITLE} + ".log";
  auto              logger   = common::LogFactory::make_default(log_name.c_str());

  auto const on_error = [&logger](auto const& error) {
    LOG_ERROR(error);
    return EXIT_FAILURE;
  };

  LOG_DEBUG("Initializing OpenGL context and SDL window.");
  auto gl_sdl = TRY_OR(GlSdl::make_default(logger, TITLE, FULLSCREEN, 1024, 768), on_error);

  auto controller = TRY_OR(SDLControllers::find_attached_controllers(logger), on_error);
  Engine engine{MOVE(gl_sdl.window), MOVE(controller)};

  LOG_DEBUG("Starting game loop");
  TRY_OR(start(logger, engine), on_error);

  LOG_DEBUG("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
