#include <cstdlib>
#include <memory>

#include <glm/glm.hpp>

#include <engine/gfx/gfx.hpp>
#include <engine/window/window.hpp>
//#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>

#include <engine/gfx/opengl_gfx.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/boomhs/boomhs.hpp>
#include <ecst.hpp>

// Define some components.
namespace c
{
} // ns c

namespace ct
{
// Define component tags.
namespace sc = ecst::signature::component;
} // ns ct

// Define some systems.
namespace s
{

struct io_system
{
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
    SDL_Event event;
    while ((! state.quit) && (0 != SDL_PollEvent(&event))) {
      state.quit = this->process_event(event, state.view, state.projection);
    }
  }
};

} // ns s

namespace st {

constexpr auto io_system = ecst::tag::system::v<s::io_system>;

} // ns st

// Setup compile-time settings.
namespace ecst_setup
{
// Builds and returns a "component signature list".
constexpr auto
make_csl()
{
  namespace sc = ecst::signature::component;
  namespace slc = ecst::signature_list::component;

  return slc::make();
}

// Builds and returns a "system signature list".
constexpr auto
make_ssl()
{
  // Signature namespace aliases.
  namespace ss = ecst::signature::system;
  namespace sls = ecst::signature_list::system;

  // Inner parallelism aliases and definitions.
  namespace ips = ecst::inner_parallelism::strategy;
  namespace ipc = ecst::inner_parallelism::composer;

  constexpr auto PA = ips::none::v();

  constexpr auto ssig_io_system = ss::make(st::io_system)
    .parallelism(PA);

  // Build and return the "system signature list".
  return sls::make(ssig_io_system);
}
} // ns ecst_setup

template<typename L, typename R>
struct game_state {
  bool quit = false;
  L &logger;
  R &renderer;
  engine::window::dimensions const dimensions;

  glm::mat4 view, projection;
public:
  game_state(L &l, R &r, engine::window::dimensions const& d) :
    logger(l),
    renderer(r),
    dimensions(d)
  {}
};

template<typename G, typename S>
void
ecst_main(G &game, S &state)
{
  // Namespace aliases.
  using namespace ecst_setup;
  namespace cs = ecst::settings;
  namespace ss = ecst::scheduler;
  namespace sea = ecst::system_execution_adapter;

  // Define ECST context settings.
  constexpr auto s = ecst::settings::make()
                        //.allow_inner_parallelism()
                        .disallow_inner_parallelism()
                        .fixed_entity_limit(ecst::sz_v<10000>)
                        .component_signatures(make_csl())
                        .system_signatures(make_ssl())
                        .scheduler(cs::scheduler<ss::s_atomic_counter>);

  auto &l = state.logger;
  l.error("pre ctx creation");

  // Create an ECST context.
  auto ctx = ecst::context::make_uptr(s);

  // Initialize context with some entities.
  ctx->step([&](auto &proxy) {
      //auto w0 = ::game::world_coordinate{-0.5f, 0.0f, 0.0f, 1.0f};
      //mk_redtriangle(proxy, w0);

      //auto w1 = game::world_coordinate{0.5f, 0.0f, 0.0f, 1.0f};
      //mk_redtriangle(proxy, w1);

      // create a single instance of the view and proj matrices
      {
        //auto eid = proxy.create_entity();
        //proxy.add_component(ct::view_m, eid);
      }
      {
        //auto eid = proxy.create_entity();
        //proxy.add_component(ct::proj_m, eid);
      }
  });

  // "Game loop."
  while (! state.quit) {

    ctx->step(
        [&state](auto &proxy) {
          proxy.execute_systems()(
              sea::t(st::io_system)
                  .for_subtasks([&state](auto &s, auto &data)
                    {
                      s.process(data, state);
                    }));
          });
    l.trace("rendering");
    game.game_loop(state);
    l.trace("game loop stepping.");
  }
}

int
main(int argc, char *argv[])
{
  auto logger = stlw::log_factory::make_default_logger("main logger");
  using L = decltype(logger);

  auto const on_error = [&](auto const &error) {
    logger.error(error);
    return EXIT_FAILURE;
  };

  // Select windowing library as SDL.
  namespace w = engine::window;
  using window_lib = w::library_wrapper<w::sdl_library>;

  logger.debug("Initializing window library globals");
  DO_MONAD_OR_ELSE_RETURN(auto _, window_lib::init(), on_error);

  logger.debug("Setting up stack guard to unitialize window library globals");
  ON_SCOPE_EXIT([]() { window_lib::destroy(); });

  auto const height = 800, width = 600;
  logger.debug("Instantiating window instance.");
  DO_MONAD_OR_ELSE_RETURN(auto window, window_lib::make_window(height, width), on_error);

  // Initialize graphics renderer
  namespace gfx = engine::gfx;
  using gfx_lib = gfx::library_wrapper<gfx::opengl::opengl_library>;
  auto const dimensions = window.get_dimensions();
  DO_MONAD_OR_ELSE_RETURN(auto renderer, gfx_lib::make_gfx_engine(logger, std::move(window)),
      on_error);
  using R = decltype(renderer);

  logger.trace("Instantiating 'state'");
  auto state = game_state<L, R>{logger, renderer, dimensions};

  // Initialize the game instance.
  logger.debug("Instantiating game 'boomhs'");
  game::boomhs::boomhs_game game;

  logger.debug("Starting game loop");
  ecst_main(game, state);

  logger.debug("Game loop finished successfully! Ending program now.");
  return EXIT_SUCCESS;
}
