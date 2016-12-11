#pragma once
#include <string>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/window/window.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/boomhs/ecst.hpp>
#include <game/boomhs/io_system.hpp>
#include <game/boomhs/randompos_system.hpp>
#include <game/data_types.hpp>

namespace game::boomhs
{

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
game::world_coordinate *s = nullptr;
game::world_coordinate *t = nullptr;
game::world_coordinate *u = nullptr;
game::world_coordinate *v = nullptr;
game::world_coordinate *w = nullptr;
game::world_coordinate *x = nullptr;
game::world_coordinate *y = nullptr;

game::world_coordinate *a = nullptr;
game::world_coordinate *b = nullptr;
game::world_coordinate *c = nullptr;

template <typename G, typename S>
void
ecst_main(G &game, S &state)
{
  auto &logger = state.logger;
  logger.trace("creating ecst context ...");

  // Create an ECST context.
  auto ctx = ecst_setup::make_context();
  logger.trace("stepping ecst once");

  // Initialize context with some entities.
  ctx->step([&](auto &proxy) {
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.2f, -0.5f, 0.0f, 1.0f);
      p = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.5f, 0.0f, 0.0f, 1.0f);
      q = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.0f, 0.7f, 0.0f, 1.0f);
      r = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.60f, -0.20f, 0.0f, 1.0f);
      s = &wc;
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
      wc.set(-0.8f, 0.55f, 0.0f, 1.0f);
      u = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.80f, -0.20f, 0.0f, 1.0f);
      v = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.80f, 0.80f, 0.0f, 1.0f);
      w = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.60f, -0.60f, 0.0f, 1.0f);
      x = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.60f, 0.60f, 0.0f, 1.0f);
      y = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.0f, 0.0f, 0.0f, 1.0f);
      a = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(-0.7f, 0.7f, 0.0f, 1.0f);
      b = &wc;
    }
    {
      auto eid = proxy.create_entity();
      auto &wc = proxy.add_component(ct::world_coordinate, eid);
      wc.set(0.7f, -0.7f, 0.0f, 1.0f);
      c = &wc;
    }
  });

  namespace sea = ecst::system_execution_adapter;
  auto io_tags = sea::t(st::io_system);
  auto randompos_tags = sea::t(st::randompos_system);

  auto const init_system = [&logger](auto &system, auto &data) {
    system.init(logger);
  };
  auto const process_system = [&state, &logger](auto &system, auto &data) {
    system.process(data, state);
  };

  auto const io_init_system = io_tags.for_subtasks(init_system);
  auto const io_process_system = io_tags.for_subtasks(process_system);

  auto const randompos_init_system = randompos_tags.for_subtasks(init_system);
  auto const randompos_process_system = randompos_tags.for_subtasks(process_system);

  auto const game_loop_body = [&](auto& proxy) {
    logger.trace("executing systems.");
    proxy.execute_systems()(io_process_system, randompos_process_system);

    logger.trace("rendering.");
    game.game_loop(state);
    logger.trace("game loop stepping.");
  };
  auto const game_loop = [&state, &game_loop_body](auto& proxy) {
    while (! state.quit) {
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

class boomhs_game
{
  NO_COPY(boomhs_game);
public:
  boomhs_game() = default;
  MOVE_DEFAULT(boomhs_game);

  template <typename State>
  void game_loop(State &state)
  {
    ::engine::gfx::render_args<decltype(state.logger)> const args{state.logger, state.view,
                                                                  state.projection};

    using COLOR_ARRAY = std::array<float, 4>;
    auto constexpr multicolor_triangle = stlw::make_array<COLOR_ARRAY>(
      stlw::concat(engine::gfx::LIST_OF_COLORS::RED, 1.0f),
      stlw::concat(engine::gfx::LIST_OF_COLORS::GREEN, 1.0f),
      stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f)
    );

    auto constexpr multicolor_rect = stlw::make_array<COLOR_ARRAY>(
        stlw::concat(engine::gfx::LIST_OF_COLORS::RED, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::GREEN, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f),
        stlw::concat(engine::gfx::LIST_OF_COLORS::YELLOW, 1.0f)
    );

    auto const height = 0.25f, width = 0.39f;
    auto triangle_color = game::triangle_factory::make(*p, ::engine::gfx::LIST_OF_COLORS::PINK);
    auto triangle_list_colors = game::triangle_factory::make(*q, multicolor_triangle);
    auto triangle_texture = game::triangle_factory::make(*r, true);
    auto triangle_wireframe = game::triangle_factory::make(*s, true, false);

    auto cube_texture = game::cube_factory::make_textured(*a, 0.25f, 0.25f, 0.25f);
    auto cube_color = game::cube_factory::make_spotted(*b, ::engine::gfx::LIST_OF_COLORS::BLUE, 0.25f, 0.25f, 0.25f);
    auto cube_wf = game::cube_factory::make_wireframe(*c, 0.25f, 0.25f, 0.25f);

    auto rectangle_color = game::rectangle_factory::make(*t, ::engine::gfx::LIST_OF_COLORS::YELLOW);
    auto rectangle_list_colors = game::rectangle_factory::make(*u, height, width, multicolor_rect);
    auto rectangle_texture = game::rectangle_factory::make(*v, height, width, true);
    auto rectangle_wireframe = game::rectangle_factory::make(*w, height, width, true, true);

    auto polygon_color = game::polygon_factory::make(*x, 5, ::engine::gfx::LIST_OF_COLORS::DARK_ORANGE);
    //auto polygon_list_of_color = game::polygon_factory::make(*u, 5, multicolor_triangle);
    auto polygon_texture = game::polygon_factory::make(*y, 7, true);
    //auto polygon_wireframe = game::polygon_factory::make(*v, 7, true, true);

    state.renderer.begin();
    state.renderer.draw_shapes_with_colors(args,
        triangle_color,
        triangle_list_colors,
        rectangle_color,
        rectangle_list_colors,
        polygon_color,
        cube_color
        //polygon_list_of_color,
        );
    state.renderer.draw_shapes_with_textures(args,
        triangle_texture,
        rectangle_texture,
        polygon_texture
        );
    state.renderer.draw_3dcube_shapes_with_textures(args,
        cube_texture
        );
    state.renderer.draw_shapes_with_wireframes(args,
        triangle_wireframe,
        rectangle_wireframe,
        //polygon_wireframe
        cube_wf
        );
    state.renderer.end();
  }
};

} // ns game::boomhs
