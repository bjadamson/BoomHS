#pragma once
#include <gfx/lib.hpp>
#include <gfx/factory.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <window/timer.hpp>

#include <stlw/random.hpp>
#include <stlw/type_ctors.hpp>

// TODO: decouple??
#include <game/boomhs/ecst.hpp>

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

  gfx::pipeline_factory pf;
  return gfx::make_shape_factories(
    pf.make_pipeline(d2.color),
    pf.make_pipeline(d2.texture_wall),
    pf.make_pipeline(d2.texture_container),
    pf.make_pipeline(d2.wireframe),

    pf.make_pipeline(d3.color),
    pf.make_pipeline(d3.texture),
    pf.make_pipeline(d3.skybox),
    pf.make_pipeline(d3.wireframe));
}

} // ns impl

template<typename L>
struct loop_state
{
  L &logger;
  bool &quit;
  gfx::camera &camera;
  glm::mat4 const& projection;

  stlw::float_generator &rnum_generator;
  std::vector<gfx::model*> &MODELS;
  gfx::model &skybox_model;
  gfx::model &terrain_model;
  window::mouse_data &mouse_data;

  gfx::render_args<L> render_args() const
  {
    return gfx::make_render_args(this->logger, this->camera, this->projection);
  }
};

template<typename L>
auto make_loop_state(L &l, bool &quit, gfx::camera &c, glm::mat4 const& p,
    stlw::float_generator &fg, std::vector<gfx::model*> &models, gfx::model &skybox,
    gfx::model &terrain, window::mouse_data &md)
{
  return loop_state<L>{l, quit, c, p, fg, models, skybox, terrain, md};
}

template<typename GFX_LIB>
class engine
{
  using W = ::window::sdl_window;
  friend struct factory;

  W window_;
public:
  GFX_LIB gfx;
private:
  engine(W &&w, GFX_LIB &&g)
      : window_(MOVE(w))
      , gfx(MOVE(g))
  {
  }
  NO_COPY(engine);
public:
  MOVE_DEFAULT(engine);

  void begin() { this->gfx.begin(); }

  template <typename G, typename S>
  void start(G &&game, S &&state)
  {
    auto &logger = state.logger;
    LOG_TRACE("creating ecst context ...");

    // Create an ECST context.
    auto ctx = ecst_setup::make_context();
    LOG_TRACE("stepping ecst once");

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
            auto const [x, y] = state.rnum_generator.generate_2dposition();
            LOG_ERROR(fmt::format("2d x, y is {}, {}", x, y));
            auto const z = 0.0f;
            state.MODELS.emplace_back(make_entity(i, glm::vec3{x, y, z}));
          }
          for (auto i{100}; i < 200; ++i) {
            auto const [x, y, z] = state.rnum_generator.generate_3dabove_ground_position();
            LOG_ERROR(fmt::format("3d x, y, z is {}, {}, {}", x, y, z));
            state.MODELS.emplace_back(make_entity(i, glm::vec3{x, y, z}));
          }
    });

    namespace sea = ecst::system_execution_adapter;
    auto io_tags = st::io_system;
    auto randompos_tags = st::randompos_system;

    auto const init_system = [&logger](auto &system, auto &) { system.init(logger); };
    auto const init = [&init_system](auto &system) { sea::t(system).for_subtasks(init_system); };

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
    auto const mls = [&mouse_data](auto &state) {
      return make_loop_state(state.logger, state.quit, state.camera, state.projection,
          state.rnum_generator, state.MODELS, state.skybox_model, state.terrain_model, mouse_data);
    };
    auto const game_loop = [&](auto &proxy) {
      auto const fn = [&]()
      {
        auto loop_state = mls(state);
        this->loop(MOVE(loop_state), game, proxy);
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
    LOG_TRACE("executing systems.");
    proxy.execute_systems()(io_process_system, randompos_process_system);

    this->begin();
    LOG_TRACE("rendering.");

    auto sf = impl::make_shape_factories(this->gfx.pipelines);
    game.game_loop(state, this->gfx, MOVE(sf));
    this->end();
    LOG_TRACE("game loop stepping.");
  }

  void end()
  {
    this->gfx.end();

    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }

  auto get_dimensions() const { return this->window_.get_dimensions(); }
};

struct factory {
  factory() = delete;
  ~factory() = delete;

  template <typename L, typename W, typename BACKEND_LIB>
  static auto
  make_engine(L &logger, W &&window, BACKEND_LIB &&backend_lib)
  {
    auto gfx_lib = gfx::lib_factory::make(logger, MOVE(backend_lib));
    using GFX_LIB = decltype(gfx_lib);
    return engine<GFX_LIB>{MOVE(window), MOVE(gfx_lib)};
  }
};

} // ns engine
