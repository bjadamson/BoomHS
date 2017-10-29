#pragma once
#include <opengl/lib.hpp>
#include <opengl/renderer.hpp>
#include <opengl/factory.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/random.hpp>
#include <stlw/type_ctors.hpp>

// TODO: decouple??
#include <game/boomhs/ecst.hpp>
#include <game/boomhs/assets.hpp>

namespace engine
{

namespace impl
{

template<typename P>
auto
make_shape_factories(P &pipelines)
{
  auto &d2 = pipelines.d2;
  auto &d3 = pipelines.d3;

  return opengl::make_shape_factories(
    d2.color,
    d2.texture_wall,
    d2.texture_container,
    d2.wireframe,

    d3.color,
    d3.wall,
    d3.texture_3dcube,
    d3.house,
    d3.skybox,
    d3.wireframe);
}

} // ns impl

template<typename L>
struct loop_state
{
  L &logger;
  bool &quit;
  opengl::camera &camera;
  glm::mat4 const& projection;

  stlw::float_generator &rnum_generator;
  std::vector<opengl::Model*> &MODELS;
  opengl::Model &terrain_model;
  opengl::Model &house_model;
  opengl::Model &skybox_model;
  window::mouse_data &mouse_data;

  opengl::render_args<L> render_args() const
  {
    return opengl::make_render_args(this->logger, this->camera, this->projection);
  }
};

template<typename L>
auto make_loop_state(L &l, bool &quit, opengl::camera &c, glm::mat4 const& p,
    stlw::float_generator &fg, std::vector<opengl::Model*> &models, opengl::Model &terrain,
    opengl::Model &house, opengl::Model &skybox, window::mouse_data &md)
{
  return loop_state<L>{l, quit, c, p, fg, models, terrain, house, skybox, md};
}

template<typename GFX_LIB>
class engine
{
  using W = ::window::sdl_window;
  friend struct engine_factory;

  W window_;
public:
  GFX_LIB lib;
private:
  explicit engine(W &&w, GFX_LIB &&gfxlib)
      : window_(MOVE(w))
      , lib(MOVE(gfxlib))
  {
  }
  NO_COPY(engine);
public:
  MOVE_DEFAULT(engine);

  template <typename G, typename S>
  void start(G &&game, S &&state)
  {
    using namespace opengl;

    auto &logger = state.logger;
    LOG_TRACE("creating ecst context ...");

    // Create an ECST context.
    auto ctx = ecst_setup::make_context();
    LOG_TRACE("stepping ecst once");

    // Initialize context with some entities.
    auto sf = impl::make_shape_factories(this->lib.pipelines);
    auto const assets = ctx->step([&](auto &proxy) {
        return game.init(proxy, state, sf);
    });

    namespace sea = ecst::system_execution_adapter;
    auto io_tags = st::io_system;
    auto randompos_tags = st::randompos_system;

    auto const init_system = [&logger](auto &system, auto &) { system.init(logger); };
    [&init_system](auto &system) { sea::t(system).for_subtasks(init_system); };

    //auto game_systems = game.ecst_systems();//MOVE(io_tags), MOVE(randompos_tags));
    //stlw::for_each(game_systems, init);

    auto const io_init_system = sea::t(io_tags).for_subtasks(init_system);
    auto const randompos_init_system = sea::t(randompos_tags).for_subtasks(init_system);

    auto mouse_data = window::make_default_mouse_data();
    int frames_counted = 0;
    window::LTimer fps_timer;
    auto const fps_capped_game_loop = [&](auto const& fn) {
      window::LTimer frame_timer;
      auto const start = frame_timer.get_ticks();
      fn();

      uint32_t const frame_ticks = frame_timer.get_ticks();
      float constexpr ONE_60TH_OF_A_FRAME = (1/60) * 1000;

      if (frame_ticks < ONE_60TH_OF_A_FRAME) {
        LOG_TRACE("Frame finished early, sleeping rest of frame.");
        SDL_Delay(ONE_60TH_OF_A_FRAME - frame_ticks);
      }

      float const fps = frames_counted / (fps_timer.get_ticks() / 1000.0f);
      LOG_INFO(fmt::format("average FPS '{}'", fps));
      ++frames_counted;
    };

    auto const game_loop = [&](auto &proxy) {
      auto const fn = [&]()
      {
        auto const mls = [&mouse_data](auto &state) {
          return make_loop_state(state.logger, state.quit, state.camera, state.projection,
              state.rnum_generator, state.MODELS, state.terrain_model, state.house_model,
              state.skybox_model, mouse_data);
        };
        auto loop_state = mls(state);
        this->loop(MOVE(loop_state), game, proxy, sf, assets);
      };
      while (!state.quit) {
        fps_capped_game_loop(fn);
      }
    };
    ctx->step([&](auto &proxy) {
      LOG_TRACE("game started, initializing systems.");
      proxy.execute_systems()(io_init_system, randompos_init_system);
      //proxy.execute_systmes()(MOVE(game_systems));
      LOG_TRACE("systems initialized, entering main loop.");

      game_loop(proxy);
      LOG_TRACE("game loop finished.");
    });
  }

  template<typename LoopState, typename Game, typename P, typename SF, typename ASSETS>
  void loop(LoopState &&state, Game &&game, P &proxy, SF &sf, ASSETS const& assets)
  {
    namespace sea = ecst::system_execution_adapter;
    auto io_tags = sea::t(st::io_system);
    auto randompos_tags = sea::t(st::randompos_system);

    auto const process_system = [&state](auto &system, auto &data) {
      system.process(data, state);
    };
    auto const io_process_system = io_tags.for_subtasks(process_system);
    auto const randompos_process_system = randompos_tags.for_subtasks(process_system);

    auto &logger = state.logger;
    LOG_TRACE("executing systems.");
    proxy.execute_systems()(io_process_system, randompos_process_system);

    this->begin();
    LOG_TRACE("rendering.");

    game.game_loop(state, sf, assets);
    this->end();
    LOG_TRACE("game loop stepping.");
  }

  void begin() { opengl::begin(); }

  void end()
  {
    opengl::end();

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }

  auto get_dimensions() const { return this->window_.get_dimensions(); }
};

struct engine_factory {
  engine_factory() = delete;
  ~engine_factory() = delete;

  template <typename L, typename W, typename GFX_LIB>
  static auto
  make_engine(L &logger, W &&window, GFX_LIB &&gfx_lib)
  {
    return engine<GFX_LIB>{MOVE(window), MOVE(gfx_lib)};
  }
};

} // ns engine
