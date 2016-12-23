#pragma once
#include <gfx/lib.hpp>
#include <window/sdl_window.hpp>
#include <stlw/random.hpp>
#include <stlw/type_ctors.hpp>

// TODO: decouple??
#include <game/boomhs/ecst.hpp>

namespace engine
{

template<typename L>
struct loop_state
{
  L &logger;
  bool &quit;
  gfx::camera &camera;
  glm::mat4 const& projection;

  // todo: ref or move-in??
  stlw::float_generator &rnum_generator;
  std::vector<gfx::model*> &MODELS;
  gfx::model &skybox_model;

  gfx::render_args<L> render_args() const
  {
    return gfx::make_render_args(this->logger, this->camera, this->projection);
  }
};

template<typename L>
auto make_loop_state(L &l, bool &quit, gfx::camera &c, glm::mat4 const& p,
    stlw::float_generator &fg, std::vector<gfx::model*> &models, gfx::model &skybox)
{
  return loop_state<L>{l, quit, c, p, fg, models, skybox};
}

class gfx_lib
{
  using W = ::window::sdl_window;
  friend struct factory;

  W window_;
public:
  gfx::gfx_lib gfx;
private:
  gfx_lib(W &&w, gfx::gfx_lib &&g)
      : window_(std::move(w))
      , gfx(std::move(g))
  {
  }
  NO_COPY(gfx_lib);
public:
  MOVE_DEFAULT(gfx_lib);

  void begin() { this->gfx.begin(); }

  template <typename G, typename S>
  void start(G &&game, S &&state)
  {
    auto &logger = state.logger;
    logger.trace("creating ecst context ...");

    // Create an ECST context.
    auto ctx = ecst_setup::make_context();
    logger.trace("stepping ecst once");

    // Initialize context with some entities.
    ctx->step([&](auto &proxy)
        {
          auto const make_entity = [&proxy](auto const i, auto const& t) {
            auto eid = proxy.create_entity();
            auto *p = &proxy.add_component(ct::model, eid);
            p->translation = t;
            return p;
          };
          for (auto i{0}; i < 100; ++i) {
            auto const x = state.rnum_generator.generate_position();
            auto const y = state.rnum_generator.generate_position();
            auto const z = 0.0f;
            logger.info(fmt::sprintf("BEN, x '%f', y '%f'", x, y));
            state.MODELS.emplace_back(make_entity(i, glm::vec3{x, y, z}));
          }
    });

    namespace sea = ecst::system_execution_adapter;
    auto io_tags = st::io_system;
    auto randompos_tags = st::randompos_system;

    auto const init_system = [&logger](auto &system, auto &) { system.init(logger); };
    auto const init = [&init_system](auto &system) { sea::t(system).for_subtasks(init_system); };

    //auto game_systems = game.ecst_systems();//std::move(io_tags), std::move(randompos_tags));
    //stlw::for_each(game_systems, init);

    auto const io_init_system = sea::t(io_tags).for_subtasks(init_system);
    auto const randompos_init_system = sea::t(randompos_tags).for_subtasks(init_system);

    auto const game_loop = [&state, &game, this](auto &proxy) {
      while (!state.quit) {
        auto loop_state = make_loop_state(state.logger, state.quit, state.camera, state.projection,
          state.rnum_generator, state.MODELS, state.skybox_model);

        this->loop(std::move(loop_state), game, proxy);
      }
    };
    ctx->step([&](auto &proxy) {
      logger.trace("game started, initializing systems.");
      proxy.execute_systems()(io_init_system, randompos_init_system);
      //proxy.execute_systmes()(std::move(game_systems));
      logger.trace("systems initialized, entering main loop.");

      game_loop(proxy);
      logger.trace("game loop finished.");
    });
  }

  template<typename LoopState, typename Game, typename P>
  void loop(LoopState &&state, Game &&game, P &proxy)
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
    logger.trace("executing systems.");
    proxy.execute_systems()(io_process_system, randompos_process_system);

    this->begin();
    logger.trace("rendering.");

    game.game_loop(state, this->gfx);
    logger.trace("game loop stepping.");
    this->end();
  }

  void end()
  {
    this->gfx.end();

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L, typename W>
  static stlw::result<gfx_lib, std::string> make_gfx_sdl_engine(L &logger, W &&window)
  {
    DO_TRY(auto gfx, gfx::factory::make_gfx_engine(logger));
    return gfx_lib{std::move(window), std::move(gfx)};
  }
};

} // ns engine
