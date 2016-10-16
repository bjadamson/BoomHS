#pragma once
#include <string>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ecst.hpp>

#include <engine/gfx/shapes.hpp>
#include <engine/window/window.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/boomhs/io_system.hpp>
#include <game/data_types.hpp>

namespace st {

constexpr auto io_system = ecst::tag::system::v<s::io_system>;

} // ns st

namespace game::boomhs
{

// Define some components.
namespace c
{
} // ns c

namespace ct
{
// Define component tags.
namespace sc = ecst::signature::component;
} // ns ct

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
                        .allow_inner_parallelism()
                        //.disallow_inner_parallelism()
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

class boomhs_game
{
  game::world_coordinate wc0_ = game::world_coordinate{-0.5f, 0.0f, 0.0f, 1.0f};
  game::world_coordinate wc1_ = game::world_coordinate{0.5f, 0.0f, 0.0f, 1.0f};

public:
  boomhs_game() = default;

  NO_COPY(boomhs_game);
  MOVE_DEFAULT(boomhs_game);

  template <typename S>
  void game_loop(S &state)
  {
    ::engine::gfx::opengl::render_args<decltype(state.logger)> const args{state.logger,
      state.view, state.projection, this->wc0_, this->wc1_};

    state.renderer.draw(args);
  }
};

} // ns game::boomhs
