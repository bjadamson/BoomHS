#pragma once
#include <string>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ecst.hpp>

#include <engine/window/window.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/boomhs/io_system.hpp>
#include <game/data_types.hpp>

namespace st
{

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

constexpr auto world_coordinate = ecst::tag::component::v<game::world_coordinate>;
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

  constexpr auto cs_world_coordinate = sc::make(ct::world_coordinate).contiguous_buffer();

  return slc::make(cs_world_coordinate);
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

  constexpr auto ssig_io_system = ss::make(st::io_system).parallelism(PA);

  // Build and return the "system signature list".
  return sls::make(ssig_io_system);
}
} // ns ecst_setup

template <typename L, typename R>
struct game_state {
  bool quit = false;
  L &logger;
  R &renderer;
  engine::window::dimensions const dimensions;

  glm::mat4 view, projection;

public:
  game_state(L &l, R &r, engine::window::dimensions const &d)
      : logger(l)
      , renderer(r)
      , dimensions(d)
  {
  }
};

// TODO: bye globals
game::world_coordinate *p = nullptr;
game::world_coordinate *q = nullptr;
game::world_coordinate *r = nullptr;
game::world_coordinate *t = nullptr;
game::world_coordinate *u = nullptr;
game::world_coordinate *v = nullptr;
game::world_coordinate *w = nullptr;
game::world_coordinate *x = nullptr;
game::world_coordinate *y = nullptr;
game::world_coordinate *z = nullptr;

template <typename G, typename S>
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
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.5f, 0.0f, 0.0f, 1.0f);
      p = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.5f, 0.0f, 0.0f, 1.0f);
      q = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.0f, 0.5f, 0.0f, 1.0f);
      r = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.0f, -0.5f, 0.0f, 1.0f);
      t = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.65f, 0.85f, 0.0f, 1.0f);
      u = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.40f, 0.00f, 0.0f, 1.0f);
      v = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.80f, 0.80f, 0.0f, 1.0f); // top left
      w = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.80f, -0.80f, 0.0f, 1.0f); // bottom left
      x = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.60f, -0.20f, 0.0f, 1.0f); // bottom left
      y = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.0f, 0.0f, 0.0f, 1.0f); // bottom left
      z = &wc;
    }
  });

  // "Game loop."
  while (!state.quit) {

    ctx->step([&state](auto &proxy) {
      proxy.execute_systems()(sea::t(st::io_system).for_subtasks([&state](auto &s, auto &data) {
        s.process(data, state);
      }));
    });
    l.debug("rendering");

    game.game_loop(state);
    l.debug("game loop stepping.");
  }
}

class boomhs_game
{
  NO_COPY(boomhs_game);

public:
  boomhs_game() = default;

  MOVE_DEFAULT(boomhs_game);

  template <typename State>    //, typename ...S>
  void game_loop(State &state) //, S const&... shapes)
  {
    ::engine::gfx::render_args<decltype(state.logger)> const args{state.logger, state.view,
                                                                  state.projection};

    using COLOR_ARRAY = std::array<float, 4>;
    auto constexpr my_color = stlw::make_array<COLOR_ARRAY>(
      stlw::concat(engine::gfx::LIST_OF_COLORS::RED, 1.0f),
      stlw::concat(engine::gfx::LIST_OF_COLORS::GREEN, 1.0f),
      stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f)
    );

    auto constexpr my_color2 = stlw::make_array<COLOR_ARRAY>(
        stlw::concat(engine::gfx::LIST_OF_COLORS::RED, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::GREEN, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::YELLOW, 1.0f)
    );

    auto constexpr my_color3 = stlw::make_array<COLOR_ARRAY>(
        stlw::concat(engine::gfx::LIST_OF_COLORS::DARK_ORANGE, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::DARK_ORANGE, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f)
    );

    auto s0 = game::rectangle_factory::make(*p, ::engine::gfx::LIST_OF_COLORS::GRAY);
    auto s1 = game::triangle_factory::make(*t, ::engine::gfx::LIST_OF_COLORS::PINK);

    auto s2 = game::triangle_factory::make(*q, my_color);

    auto const height = 0.25f, width = 0.39f;
    auto s3 = game::rectangle_factory::make(*r, my_color2, height, width);

    auto s4 = game::polygon_factory::make(*u, 5, ::engine::gfx::LIST_OF_COLORS::DARK_ORANGE);
    auto s5 = game::polygon_factory::make(*v, 7, true);

    auto s6 = game::triangle_factory::make(*w, true);
    auto s7 = game::rectangle_factory::make(*x, ::engine::gfx::LIST_OF_COLORS::BLACK, height, width);
    auto s8 = game::rectangle_factory::make(*y, height, width, true);
    auto s9 = game::triangle_factory::make(*z, true, false); // wireframe

    state.renderer.begin();
    state.renderer.draw0(args, s0, s1, s2, s3, s4, s7);
    state.renderer.draw1(args, s5, s6, s8);
    state.renderer.draw2(args, s9);
    state.renderer.end();
  }
};

} // ns game::boomhs
