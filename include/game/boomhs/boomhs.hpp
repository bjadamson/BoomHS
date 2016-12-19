#pragma once
#include <string>
#include <vector>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/window/window.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/skybox.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/burrito.hpp>
#include <stlw/random.hpp>
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
  stlw::float_generator rnum_generator;

  glm::mat4 projection;
  engine::gfx::camera camera;

  NO_COPY(game_state);
  NO_MOVE_ASSIGN(game_state);
public:
  MOVE_CONSTRUCTIBLE(game_state);
  game_state(L &l, R &r, engine::window::dimensions const &d, stlw::float_generator &&fg,
      glm::mat4 &&pm, engine::gfx::camera &&c)
    : logger(l)
    , renderer(r)
    , dimensions(d)
    , rnum_generator(std::move(fg))
    , projection(std::move(pm))
    , camera(std::move(c))
  {
  }
};

// TODO: bye globals
std::vector<game::model*> MODELS;
game::model skybox_model;

template<typename L, typename R, typename HW>
auto
make_state(L &logger, R &renderer, HW const& hw)
{
  auto const fheight = static_cast<GLfloat>(hw.h);
  auto const fwidth = static_cast<GLfloat>(hw.w);

  logger.error(fmt::sprintf("fheight '%f', fwidth '%f'", fheight, fwidth));
  auto projection = glm::perspective(45.0f, (fwidth / fheight), 0.1f, 10000.0f);
  engine::gfx::skybox sb{skybox_model};
  engine::gfx::camera c{std::move(sb),
    glm::vec3(0.0f, 0.0f, 2.0f),  // camera position
    glm::vec3(0.0f, 0.0f, -1.0f), // look at origin
    glm::vec3(0.0f, 1.0f, 0.0f)}; // "up" vector
  stlw::float_generator rng;
  return game_state<L, R>(logger, renderer, hw, std::move(rng), std::move(projection), std::move(c));
}

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
  ctx->step([&](auto &proxy)
      {
        auto const make_entity = [&proxy](auto const i, auto const& t) {
          auto eid = proxy.create_entity();
          auto *p = &proxy.add_component(ct::model, eid);
          p->translation = t;
          return p;
        };
        for (auto i{0}; i < 10; ++i) {
          auto const x = state.rnum_generator.generate_position();
          auto const y = state.rnum_generator.generate_position();
          auto const z = 0.0f;
          logger.info(fmt::sprintf("BEN, x '%f', y '%f'", x, y));
          MODELS.emplace_back(make_entity(i, glm::vec3{x, y, z}));
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

    // first, pick random shape
    auto const random_mode = [&]() {
      return static_cast<drawmode>(drawmode::TRIANGLES);
    };

    auto const random_comp = [&]() { return state.rnum_generator.generate_0to1(); };
    auto const random_color = [&]() {
      return glm::vec4{random_comp(), random_comp(), random_comp(), 1.0f};
    };

    /*
    auto const height = 0.25f, width = 0.39f;
    auto triangle_color = game::triangle_factory::make(drawmode::TRIANGLES, *pmv, ::engine::gfx::LIST_OF_COLORS::PINK);
    auto triangle_list_colors = game::triangle_factory::make(drawmode::TRIANGLES, *qmv, multicolor_triangle);
    auto triangle_texture = game::triangle_factory::make(drawmode::TRIANGLES, *rmv, true);
    auto triangle_wireframe = game::triangle_factory::make(drawmode::LINE_LOOP, *smv, true, false);

    auto cube_texture = game::cube_factory::make_textured(drawmode::TRIANGLE_STRIP, *amv, {0.15f, 0.15f, 0.15f});
    auto cube_skybox = game::cube_factory::make_textured(drawmode::TRIANGLE_STRIP, skybox_model, {15.0f, 15.0f, 15.0f});
    auto cube_color = game::cube_factory::make_spotted(drawmode::TRIANGLE_STRIP, *bmv, ::engine::gfx::LIST_OF_COLORS::BLUE,
        {0.25f, 0.25f, 0.25f});
    auto cube_wf = game::cube_factory::make_wireframe(drawmode::LINE_LOOP, *cmv, {0.25f, 0.25f, 0.25f});

    auto rectangle_color = game::rectangle_factory::make(drawmode::TRIANGLE_STRIP, *tmv, ::engine::gfx::LIST_OF_COLORS::YELLOW);
    auto rectangle_list_colors = game::rectangle_factory::make(drawmode::TRIANGLE_STRIP, *umv, height, width, multicolor_rect);
    auto rectangle_texture = game::rectangle_factory::make(drawmode::TRIANGLE_STRIP, *vmv, height, width, true);
    auto rectangle_wireframe = game::rectangle_factory::make(drawmode::LINE_LOOP, *wmv, height, width, true, true);

    auto polygon_color =
        game::polygon_factory::make(drawmode::TRIANGLE_FAN, *xmv, 5, ::engine::gfx::LIST_OF_COLORS::DARK_ORANGE);
    auto polygon_texture = game::polygon_factory::make(drawmode::TRIANGLE_FAN, *ymv, 7, true);
    auto polygon_wireframe = game::polygon_factory::make(drawmode::LINE_LOOP, *zmv, 7, true, true);
    */
    //auto polygon_list_of_color = game::polygon_factory::make(drawmode::TRIANGLE_FAN, *zp1, 5, multicolor_triangle);

    auto &r = state.renderer;
    auto &d2 = r.engine.d2;
    auto &d3 = r.engine.d3;
    r.begin();

    std::array<game::triangle<game::vertex_color_attributes>, 4> const arr = {
      game::triangle_factory::make(random_mode(), *MODELS[0], random_color()),
      game::triangle_factory::make(random_mode(), *MODELS[1], random_color()),
      game::triangle_factory::make(random_mode(), *MODELS[2], random_color()),
      game::triangle_factory::make(random_mode(), *MODELS[3], random_color()),
    };

    //r.draw(args, d2.color, stlw::make_burrito(arr.cbegin(), arr.cend()));

    //r.draw(args, d2.color, stlw::tuple_from_array(arr));
    r.draw(args, d2.color, stlw::make_burrito(game::triangle_factory::make(random_mode(), *MODELS[4], random_color()),
      game::triangle_factory::make(random_mode(), *MODELS[5], random_color())));

    /*
    game::triangle<game::vertex_color_attributes> const arr2[2] = {
      game::triangle_factory::make(random_mode(), *MODELS[8], random_color()),
      game::triangle_factory::make(random_mode(), *MODELS[9], random_color())
    };
    r.draw(args, d2.color, stlw::tuple_from_array(arr2));
    */

    //std::vector<game::triangle<game::vertex_color_attributes>> vec;
    //vec.emplace_back(game::triangle_factory::make(random_mode(), *MODELS[6], random_color()));
    //vec.emplace_back(game::triangle_factory::make(random_mode(), *MODELS[7], random_color()));

    //auto const bURR = stlw::make_burrito(vec);
    //r.draw(args, d2.color, bURR);

    //int x = game::triangle_factory::make(drawmode::TRIANGLES, *(MODELS[0]), random_color());
    //r.draw(args, d2.color, );
    //m[1] = game::triangle_factory::make(random_mode(), *MODELS[1], random_color());

    //r.draw(args, d2.color, *MODELS[0], *MODELS[1]);

    /*
    r.draw(args, d3.skybox, cube_skybox);

    r.draw(args, d2.color, triangle_color, triangle_list_colors,
                                           rectangle_color, rectangle_list_colors, polygon_color
    //                                        polygon_list_of_color,
                                           );
    r.draw(args, d2.texture_wall, triangle_texture, rectangle_texture, polygon_texture);
    r.draw(args, d2.texture_container, polygon_texture);
    r.draw(args, d2.wireframe, polygon_wireframe, triangle_wireframe, rectangle_wireframe);

    r.draw(args, d3.color, cube_color);
    r.draw(args, d3.texture, cube_texture);
    r.draw(args, d3.wireframe, cube_wf);
    */
    state.renderer.end();
  }
};

} // ns game::boomhs
