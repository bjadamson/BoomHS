#pragma once
#include <gfx/lib.hpp>
#include <window/sdl_window.hpp>
#include <stlw/type_ctors.hpp>
#include <game/boomhs/ecst.hpp>

namespace engine
{

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
    auto io_tags = sea::t(st::io_system);
    auto randompos_tags = sea::t(st::randompos_system);

    auto const init_system = [&logger](auto &system, auto &data) { system.init(logger); };
    auto const process_system = [&state, &logger](auto &system, auto &data) {
      system.process(data, state);
    };

    auto const io_init_system = io_tags.for_subtasks(init_system);
    auto const io_process_system = io_tags.for_subtasks(process_system);

    auto const randompos_init_system = randompos_tags.for_subtasks(init_system);
    auto const randompos_process_system = randompos_tags.for_subtasks(process_system);

    auto const game_loop_body = [&](auto &proxy) {
      logger.trace("executing systems.");
      proxy.execute_systems()(io_process_system, randompos_process_system);

      this->begin();
      logger.trace("rendering.");
      game.game_loop(state, this->gfx);
      logger.trace("game loop stepping.");
      this->end();
    };
    auto const game_loop = [&state, &game_loop_body](auto &proxy) {
      while (!state.quit) {
        game_loop_body(proxy);
      }
    };
    ctx->step([&](auto &proxy) {
      logger.trace("game started, initializing systems.");
      proxy.execute_systems()(io_init_system, randompos_init_system);
      logger.trace("systems initialized, entering main loop.");

      game_loop(proxy);
      logger.trace("game loop finished.");
    });
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
