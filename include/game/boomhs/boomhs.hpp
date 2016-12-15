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

  glm::mat4 projection;
  engine::gfx::camera camera;

  NO_COPY(game_state);
  NO_MOVE_ASSIGN(game_state);
public:
  MOVE_CONSTRUCTIBLE(game_state);
  game_state(L &l, R &r, engine::window::dimensions const &d, glm::mat4 &&pm,
      engine::gfx::camera &&c)
    : logger(l)
    , renderer(r)
    , dimensions(d)
    , projection(std::move(pm))
    , camera(std::move(c))
  {
  }
};

template<typename L, typename R, typename HW>
auto
make_state(L &logger, R &renderer, HW const& hw)
{
  auto const fheight = static_cast<GLfloat>(hw.h);
  auto const fwidth = static_cast<GLfloat>(hw.w);

  logger.error(fmt::sprintf("fheight '%f', fwidth '%f'", fheight, fwidth));
  auto projection = glm::perspective(45.0f, (fwidth / fheight), 0.1f, 10000.0f);
  engine::gfx::camera c(
    glm::vec3(0.0f, 0.0f, 2.0f), // camera position
    glm::vec3(0.0f, 0.0f, -1.0f),  // look at origin
    glm::vec3(0.0f, 1.0f, 0.0f)); // "up" vector

  return game_state<L, R>(logger, renderer, hw, std::move(projection), std::move(c));
}

// TODO: bye globals
game::model *pmv = nullptr;
game::model *qmv = nullptr;
game::model *rmv = nullptr;
game::model *smv = nullptr;
game::model *tmv = nullptr;
game::model *umv = nullptr;
game::model *vmv = nullptr;
game::model *wmv = nullptr;
game::model *xmv = nullptr;
game::model *ymv = nullptr;
game::model *amv = nullptr;
game::model *bmv = nullptr;
game::model *cmv = nullptr;

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
      pmv = &proxy.add_component(ct::model, eid);
      pmv->translation = glm::vec3{-0.2f, -0.5f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      qmv = &proxy.add_component(ct::model, eid);
      qmv->translation = glm::vec3{-0.5f, 0.0f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      rmv = &proxy.add_component(ct::model, eid);
      rmv->translation = glm::vec3{0.0f, 0.7f, -1.0f};
    }
    {
      auto eid = proxy.create_entity();
      smv = &proxy.add_component(ct::model, eid);
      smv->translation = glm::vec3{0.60f, -0.20f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      tmv = &proxy.add_component(ct::model, eid);
      tmv->translation = glm::vec3{0.0f, -0.5f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      umv = &proxy.add_component(ct::model, eid);
      umv->translation = glm::vec3{-0.8f, 0.55f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      vmv = &proxy.add_component(ct::model, eid);
      vmv->translation = glm::vec3{0.80f, -0.20f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      wmv = &proxy.add_component(ct::model, eid);
      wmv->translation = glm::vec3{-0.80f, 0.80f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      xmv = &proxy.add_component(ct::model, eid);
      xmv->translation = glm::vec3{-0.60f, -0.60f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      ymv = &proxy.add_component(ct::model, eid);
      ymv->translation = glm::vec3{0.60f, 0.60f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      amv = &proxy.add_component(ct::model, eid);
      amv->translation = glm::vec3{0.0f, 0.0f, 0.0f};
    }
    {
      auto eid = proxy.create_entity();
      bmv = &proxy.add_component(ct::model, eid);
      bmv->translation = glm::vec3{0.7f, 0.0f, 0.0f};
    }
    /*
    {
      auto eid = proxy.create_entity();
      cmv = &proxy.add_component(ct::model, eid);
      cmv->translation = glm::vec3{0.7f, -0.7f, 0.0f};
    }
    */
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

    logger.trace("rendering.");
    game.game_loop(state);
    logger.trace("game loop stepping.");
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

class boomhs_game
{
  NO_COPY(boomhs_game);

public:
  boomhs_game() = default;
  MOVE_DEFAULT(boomhs_game);

  template <typename State>
  void game_loop(State &state)
  {
    ::engine::gfx::render_args<decltype(state.logger)> const args{state.logger, state.camera,
                                                                  state.projection};

    using COLOR_ARRAY = std::array<float, 4>;
    auto constexpr multicolor_triangle =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(engine::gfx::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(engine::gfx::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f));

    auto constexpr multicolor_rect =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(engine::gfx::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(engine::gfx::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(engine::gfx::LIST_OF_COLORS::BLUE, 1.0f),
                                      stlw::concat(engine::gfx::LIST_OF_COLORS::YELLOW, 1.0f));

    auto const height = 0.25f, width = 0.39f;
    auto triangle_color = game::triangle_factory::make(*pmv, ::engine::gfx::LIST_OF_COLORS::PINK);
    auto triangle_list_colors = game::triangle_factory::make(*qmv, multicolor_triangle);
    auto triangle_texture = game::triangle_factory::make(*rmv, true);
    //auto triangle_wireframe = game::triangle_factory::make(*smv, true, false);

    auto cube_texture = game::cube_factory::make_textured(*amv, {0.15f, 0.15f, 0.15f});
    auto cube_color = game::cube_factory::make_spotted(*bmv, ::engine::gfx::LIST_OF_COLORS::BLUE,
        {0.25f, 0.25f, 0.25f});
    //auto cube_wf = game::cube_factory::make_wireframe(*cmv, {0.25f, 0.25f, 0.25f});

    auto rectangle_color = game::rectangle_factory::make(*tmv, ::engine::gfx::LIST_OF_COLORS::YELLOW);
    auto rectangle_list_colors = game::rectangle_factory::make(*umv, height, width, multicolor_rect);
    auto rectangle_texture = game::rectangle_factory::make(*vmv, height, width, true);
    //auto rectangle_wireframe = game::rectangle_factory::make(*wmv, height, width, true, true);

    auto polygon_color =
        game::polygon_factory::make(*xmv, 5, ::engine::gfx::LIST_OF_COLORS::DARK_ORANGE);
    //auto polygon_list_of_color = game::polygon_factory::make(*u, 5, multicolor_triangle);
    auto polygon_texture = game::polygon_factory::make(*ymv, 7, true);
    //auto polygon_wireframe = game::polygon_factory::make(*v, 7, true, true);

    auto &r = state.renderer;
    auto &d2 = r.engine.d2;
    auto &d3 = r.engine.d3;
    r.begin();
    r.draw(args, d2.color, triangle_color, triangle_list_colors,
                                           rectangle_color, rectangle_list_colors, polygon_color
                                           // polygon_list_of_color,
                                           );
    r.draw(args, d2.texture_wall, triangle_texture, rectangle_texture,
                                             polygon_texture
                                             );

    r.draw(args, d2.texture_container,
                                             polygon_texture
                                             );

    r.draw(args, d3.color, cube_color);
    r.draw(args, d3.texture, cube_texture);
    //state.renderer.draw_shapes_with_wireframes(args, //triangle_wireframe, rectangle_wireframe,
                                               // polygon_wireframe
                                               //cube_wf);
    state.renderer.end();
  }
};

} // ns game::boomhs
